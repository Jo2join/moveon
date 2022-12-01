#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
// #include <websocketpp/transport/asio/endpoint.hpp>
#include <websocketpp/server.hpp>
#include <boost/bind/bind.hpp>
#include <map>
#include <map>
#include <thread>
#include <memory>
#include <iostream>
#include <atomic>
#include "aaws_session.h"

namespace DCS
{

    using Server = websocketpp::server<websocketpp::config::asio>;
    typedef websocketpp::lib::asio::io_service ws_service;

    class websocket_server
    {
        friend class aaws_session;
    public:
        static websocket_server &instance()
        {
            static auto sv = std::make_shared<websocket_server>();
            return *sv.get();
        }

        websocket_server() 
        : m_pserver(std::make_shared<Server>())
        {
        }

        enum server_state
        {
            UNINIT,
            INITED,
            RUNNING,
            ERROR
        };

        ~websocket_server() = default;

        bool init_service();

        bool is_conn_valid(uint64_t conn_addr);

        // 此服务是当前程序的主要功能 ，失败则退出此进程
        void run();
        void exit();

        server_state get_server_state();

    protected:
        void on_async_send(websocketpp::connection_hdl hdl, std::string &msg, websocketpp::frame::opcode::value opcode);
        void start_async_thread(unsigned int n);
        void on_open(websocketpp::connection_hdl hdl);
        void on_close(websocketpp::connection_hdl hdl);
        void on_message(websocketpp::connection_hdl hdl, Server::message_ptr msg);
        bool on_ping(websocketpp::connection_hdl hdl, std::string payload);
        bool on_pong(websocketpp::connection_hdl hdl, std::string payload);
        bool on_validate(websocketpp::connection_hdl hdl);
        void dump_msg();

    private:
        std::shared_ptr<Server> m_pserver;

        std::map<std::thread::id,std::shared_ptr<ws_service>> m_io_service;
        
        std::vector<std::shared_ptr<ws_service::work>> m_work;

        bool verify_token(const std::string &connecter, const std::string &token);

        bool m_exit{false};

        std::atomic_flag m_flag{ATOMIC_FLAG_INIT};

        server_state m_ss{server_state::UNINIT};

        std::map<uint64_t,std::string> m_connecter;

        int _port{9090};

        int _async_thread_num{5};
        
        };
};
