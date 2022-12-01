#include <iostream>
#include <thread>
#include "aaws_session.h"
#include "websocket_server.h"


namespace DCS
{

    std::atomic<uint32_t> DCS::aaws_session::m_id(0);

    bool aaws_session::valid()
    {
        return websocket_server::instance().is_conn_valid(m_conn_addr);
    }


    void aaws_session::async_send(const std::string &msg, int opcode)
    {
        std::cout << "tid [" << std::this_thread::get_id() << "]  async_send post\n";
        auto &s = websocket_server::instance();
        auto iter = s.m_io_service.find(m_tid);
        if(iter != s.m_io_service.end())
        {
            (iter->second)->post(boost::bind(&websocket_server::on_async_send, &s, m_hdl, msg, websocketpp::frame::opcode::value(opcode)));
        }
        return;
    }

    int aaws_session::send(const std::string &msg, int opcode)
    {
        auto server_ptr = m_pserver.lock();
        if(!server_ptr.get())
        {
            return -1;
        }
        websocketpp::lib::error_code ec;
	    server_ptr->send(m_hdl,msg,websocketpp::frame::opcode::value(opcode),ec);
        if(ec)
        {
            std::cout << "Error sending message: " << ec.message() << std::endl;
            return ec.value();
        }
        return 0;
    }
} // namespace DCS
