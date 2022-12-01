#pragma once
#include <thread>
#include <functional>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <iostream>
#include <array>
#include <vector>
#include <map>
#include <sys/sysinfo.h>

namespace DCS
{
    class thread_pool : public std::enable_shared_from_this<thread_pool>
    {
    public:
        thread_pool() : m_current_run_thread(0), m_thread_num(0), m_exit(false) {}

        virtual ~thread_pool() {}

        std::shared_ptr<thread_pool> get_self()
        {
            return shared_from_this();
        }

        bool init_thread_pool()
        {
            if (m_inited)
            {
                return true;
            }
            auto core_num = get_nprocs();
            std::cout << "thread pool init! thread count:" << core_num << std::endl;
            while (core_num--)
            {
                std::thread t(thread_pool::run, weak_from_this());
                t.detach();
            }
            m_thread_num = core_num;
            m_inited = true;
            return true;
        }
        bool thread_pool_inited() const
        {
            return m_inited;
        }
        // 多线程资源释放，不在析构函数内部处理。 release 为阻塞操作，直到所有的线程全部退出
        void release()
        {
            m_exit.exchange(true);
            m_condition.notify_all();
            std::unique_lock<std::mutex> locker(m_mutex);
            while (m_current_run_thread > 0)
            {
#define WAIT_EXITED_SLEEP_TIME 100
                locker.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_EXITED_SLEEP_TIME));
                locker.lock();
            }
        }
        enum task_type
        {
            timer_task,            // 定时任务的优先级是高于普通任务的，低于最高优先级任务
            normal_task,           // 原则上优先级最低
            highest_priority_task, // 保证当前的任务，立刻开启处理
        };
        template <typename func, typename... args>
        void add_task(task_type tt, const func &f, args... a)
        {
            std::unique_lock<std::mutex> locker(m_mutex);
            // m_normal_task_queue.push(std::bind(f,std::forward<args>(a)...));
            //
            switch (tt)
            {
            case highest_priority_task:
                if (m_thread_num == m_current_run_thread)
                {
                    // 所有的线程都是忙碌状态，直接开启新的线程处理它
                    std::thread t(f, std::forward<args>(a)...);
                    t.detach();
                }
                else
                {
                    m_hpest_task_queue.push(std::bind(f, std::forward<args>(a)...));
                }
                break;
            case normal_task:
                m_normal_task_queue.push(std::bind(f, std::forward<args>(a)...));
                break;
            case timer_task:
                m_timer_task_queue.push(std::bind(f, std::forward<args>(a)...));
                break;
            default:
                break;
            }
            locker.unlock();
            m_condition.notify_all();
        }

    protected:
        static void run(std::weak_ptr<thread_pool> ptr)
        {
            auto shared_p = ptr.lock();
            if (!shared_p.get())
            {
                return;
            }
            std::unique_lock<std::mutex> locker(shared_p->m_mutex);
            std::function<void()> func;
            shared_p->m_condition.wait(locker, [&]()
                                       { return shared_p->get_task(func) || shared_p->m_exit; });
            if (shared_p->m_exit)
            {
                return;
            }
            shared_p->m_current_run_thread++;
            locker.unlock();
            func();
            locker.lock();
            shared_p->m_current_run_thread--;
            return;
        }
        bool get_task(std::function<void()> &func)
        {
            if (!m_hpest_task_queue.empty())
            {
                func = m_hpest_task_queue.front();
                m_hpest_task_queue.pop();
                return true;
            }

            if (!m_timer_task_queue.empty())
            {
                func = m_timer_task_queue.front();
                m_timer_task_queue.pop();
                return true;
            }

            if (m_normal_task_queue.empty())
            {
                return false;
            }
            func = m_normal_task_queue.front();
            m_normal_task_queue.pop();
            return true;
        }

    private:
        int m_current_run_thread;
        int m_thread_num;
        std::mutex m_mutex;
        std::atomic_bool m_exit;
        std::condition_variable m_condition;
        std::queue<std::function<void()>> m_normal_task_queue;
        std::queue<std::function<void()>> m_timer_task_queue;
        std::queue<std::function<void()>> m_hpest_task_queue;

        bool m_inited{false};
    };
    class thread_pool_global_intance
    {
    public:
        static thread_pool &intance()
        {
            static std::shared_ptr<thread_pool> th = std::make_shared<thread_pool>();
            return *th.get();
        }
    };

    class time_wheel : public std::enable_shared_from_this<time_wheel>
    {
    public:
        struct task
        {
            int count;
            uint64_t id;
            bool valid;
            std::function<void()> task;
        };

        time_wheel() = default;
        virtual ~time_wheel() = default;

        void init_wheel()
        {
            std::thread t([&]()
                          {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        thread_pool_global_intance::intance().add_task(thread_pool::highest_priority_task,[](std::weak_ptr<time_wheel> tw){
            auto ptr = tw.lock();
            if(ptr.get())
            {
                std::unique_lock locker(ptr->m_mutex);
                ptr->turn();
                ptr->process();
            }
        },weak_from_this()); });
            t.detach();
            m_inited = true;
        }
        bool is_inited() const
        {
            return m_inited;
        }

        // 如果任务已经处理返回false，否则返回true。
        bool cancel_timer(uint64_t id)
        {
            std::unique_lock<std::mutex> m_mutex;
            for (auto &i : m_task_map[m_id_map[id]])
            {
                if (i.id == id)
                {
                    if (i.count < 0)
                    {
                        return false;
                    }
                    else
                    {
                        i.valid = false;
                        return true;
                    }
                    break;
                }
            }
            return false;
        }

        template <typename func, typename... args>
        uint64_t add_s_task(int s, const func &f, args... a)
        {
            return add_ms_task(1000 * s, f, std::forward<args>(a)...);
        }
        template <typename func, typename... args>
        uint64_t add_ms_task(int ms, const func &f, args... a)
        {
            std::unique_lock locker(m_mutex);
            task t;
            m_cid++;
            auto ms_timer = ms + m_current_scale;
            t.count = ms_timer / m_scale_count;
            t.task = std::bind(f, std::forward<args>(a)...);
            t.id = m_cid;
            t.valid = true;
            // 有过期的即复用，没过期的即增加
            bool insert{false};
            for (auto &i : m_task_map[ms_timer % m_scale_count])
            {
                if (i.count < 0)
                {
                    i = t;
                    insert = true;
                    break;
                }
            }
            if (!insert)
            {
                m_task_map[ms_timer % m_scale_count].push_back(t);
            }
            m_id_map[m_cid] = ms_timer % m_scale_count;
            return m_cid;
        }

    private:
        friend class thread_pool;
        inline void turn()
        {
            if (m_current_scale == m_scale_count)
            {
                m_current_scale = 1;
            }
            else
            {
                m_current_scale++;
            }
        }
        inline void process()
        {
            auto &task_list = m_task_map[m_current_scale];
            for (auto &i : task_list)
            {
                if (i.count == 0 && i.valid == true)
                {
                    thread_pool_global_intance::intance().add_task(thread_pool::timer_task, i.task);
                }
                i.count--;
            }
        }
        std::mutex m_mutex;
        bool m_inited{false};
        int m_current_scale{1};
        const int m_scale_count{100};
        uint64_t m_cid{0};
        std::map<int, std::vector<task>> m_task_map;
        std::map<uint64_t, int> m_id_map; // 暂时没办法直接找到相应的任务，所以在任务集合中先一步找到小任务集
    };

    class time_wheel_global_intance
    {
    public:
        static time_wheel &intance()
        {
            static std::shared_ptr<time_wheel> tw = std::make_shared<time_wheel>();
            if (!tw->is_inited())
            {
                tw->init_wheel();
            }
            return *tw.get();
        }
    };

};
