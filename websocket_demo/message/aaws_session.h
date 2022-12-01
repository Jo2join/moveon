#ifndef _AAWS_SESSION_H__
#define _AAWS_SESSION_H__

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <boost/bind/bind.hpp>
#include <websocketpp/common/memory.hpp>
#include <memory>
#include <atomic>
#include "aaws_message.h"

namespace DCS
{

    class aaws_session
    {
        typedef websocketpp::server<websocketpp::config::asio> Server;
        typedef websocketpp::lib::asio::io_service ws_service;
    public:

        aaws_session(websocketpp::connection_hdl hdl
                    ,std::weak_ptr<Server> s
                    ,const std::thread::id &tid)
                    :m_hdl(hdl), m_status("Connecting"), m_pserver(s), m_tid(tid)
        {
            auto con = m_pserver.lock()->get_con_from_hdl(hdl);
            m_conn_addr = (uint64_t)std::addressof(*con.get());
            m_id++;
        }
/*
        void on_open(client *c, websocketpp::connection_hdl hdl)
        {
            m_status = "Open";

            client::connection_ptr con = c->get_con_from_hdl(hdl);
            m_server = con->get_response_header("Server");
        }

        void on_fail(client *c, websocketpp::connection_hdl hdl)
        {
            m_status = "Failed";

            client::connection_ptr con = c->get_con_from_hdl(hdl);
            m_server = con->get_response_header("Server");
            m_error_reason = con->get_ec().message();
        }

        void on_close(client *c, websocketpp::connection_hdl hdl)
        {
            m_status = "Closed";
            client::connection_ptr con = c->get_con_from_hdl(hdl);
            std::stringstream s;
            s << "close code: " << con->get_remote_close_code() << " ("
              << websocketpp::close::status::get_string(con->get_remote_close_code())
              << "), close reason: " << con->get_remote_close_reason();
            m_error_reason = s.str();
        }
*/
        inline std::string get_status()
        {
            return m_status;
        }

        inline websocketpp::connection_hdl &get_hdl()
        {
            return m_hdl;
        }

        inline uint32_t get_id()
        {
            return m_id.load();
        }

        inline uint64_t conn_id() {return m_conn_addr;}

        bool valid();

        void async_send(const std::string &msg, int opcode = 0x2 /*binary*/);

        int send(const std::string &msg, int opcode = 0x2 /*binary*/);

    private:
        static std::atomic<uint32_t> m_id;
        websocketpp::connection_hdl m_hdl;
        std::string m_status;
        std::weak_ptr<Server> m_pserver;
        std::string m_error_reason;
        std::thread::id m_tid;
        uint64_t m_conn_addr;
    };

    typedef std::shared_ptr<aaws_session> session_sptr;
    typedef std::weak_ptr<aaws_session> session_wptr;

} // namespace DCS

#endif