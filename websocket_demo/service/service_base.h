#ifndef _SERVICE_BASE_H__
#define _SERVICE_BASE_H__

#include <string>
#include <map>
#include <memory>
#include <functional>
#include "aaws_session.h"
#include "service_mgr.h"

namespace DCS
{
    class service_base
    {
    private:
    public:
        service_base() {}
        ~service_base() {}

        /// @brief ws消息处理函数，由具体服务类实现
        /// @param session 服务处理会话信息 包含ws消息和回复接口
        /// @return 0表示成功 其他表示失败
        virtual int doService(session_sptr session, aaws_msg_sptr msg_ptr) = 0;

        /// @brief 获取服务唯一uri，用于服务路由
        /// @return 服务唯一uri
        virtual std::string uri() {return _uri; }

        void setUri(const std::string &uri) {_uri = uri;}

        /// @brief 服务开启/注册
        /// @return 0 成功 1 失败
        inline int start()
        {
            return service_mgr::get_service_mgr().regist(this);
        }

        /// @brief 服务关闭/取消注册
        /// @return 0 成功 1 失败
        inline int stop()
        {
            return service_mgr::get_service_mgr().unregist(this->uri());
        }

    private:
        std::string _uri{"default"};
    };

} // namespace DCS

#endif
