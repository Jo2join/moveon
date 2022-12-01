#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <functional>
#include <chrono>
#include <vector>
#include <memory>
#include <cstring>
#include "websocket_server.h"
#include "thread_pool.h"
#include "service_mgr.h"


using namespace DCS;

bool websocket_server::init_service()
{
    m_pserver->set_reuse_addr(true); 
	m_pserver->clear_access_channels(websocketpp::log::alevel::all);
	m_pserver->clear_error_channels(websocketpp::log::elevel::all);
    // m_pserver->set_access_channels(websocketpp::log::alevel::all^websocketpp::log::alevel::frame_payload);
	// m_pserver->set_access_channels(websocketpp::log::alevel::all);

	m_pserver->init_asio();
	
	m_pserver->set_ping_handler(std::bind(&websocket_server::on_ping,this,std::placeholders::_1,std::placeholders::_2));
	m_pserver->set_pong_handler(std::bind(&websocket_server::on_pong,this,std::placeholders::_1,std::placeholders::_2));
	m_pserver->set_open_handler(std::bind(&websocket_server::on_open,this,std::placeholders::_1));
	m_pserver->set_close_handler(std::bind(&websocket_server::on_close,this,std::placeholders::_1));
	m_pserver->set_message_handler(std::bind(&websocket_server::on_message,this,std::placeholders::_1,std::placeholders::_2));
	// // 连接验证
	m_pserver->set_validate_handler(std::bind(&websocket_server::on_validate,this,std::placeholders::_1));
	
	m_ss = server_state::INITED;
    return true;
}

void websocket_server::start_async_thread(unsigned int n)
{
	for (unsigned int i = 0; i < n; i++)
	{
		std::thread t([&]()
			{
				std::shared_ptr<ws_service> ios = std::make_shared<ws_service>();
				m_io_service.insert({std::this_thread::get_id(),ios});
				m_work.push_back( std::make_shared<ws_service::work>(*ios.get()) );
				while(1)
				{
					m_pserver->poll();///用于监听socket消息
					ios->poll();///用于监听post消息
					usleep(1000);
				} 
			}
		);
		t.detach();
	}
}

void websocket_server::dump_msg()
{
	for(auto &m : m_io_service)
	{
		std::cout 
		<<"websocket server listen port: "
		<< _port
		<< " tid: " << m.first << std::endl;
	}
}

void websocket_server::run()
{
	try 
	{
		m_pserver->listen(_port);

		//Start the server accept loop
		m_pserver->start_accept();

		//Start the ASIO io_service run loop
		m_ss = server_state::RUNNING;
	}
	catch (websocketpp::exception const &e) 
	{
		std::cout << "error: " << e.what() << std::endl;
		m_ss = server_state::ERROR;
	}

	start_async_thread(_async_thread_num);
	
	if(1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		dump_msg();
	}

	return;
}

void websocket_server::exit()
{
    m_pserver->stop();
	while(true)
	{
 		if(m_pserver->stopped())
		{
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	m_exit = true;
}

websocket_server::server_state websocket_server::get_server_state()
{
	return m_ss;
}

bool websocket_server::is_conn_valid(uint64_t conn_addr)
{
	bool res = false;
	while(m_flag.test_and_set());
	res = (m_connecter.end() != m_connecter.find(conn_addr));
	m_flag.clear();
	return res;
}

void websocket_server::on_async_send(websocketpp::connection_hdl hdl, std::string &msg, websocketpp::frame::opcode::value opcode)
{
	std::cout << "tid [" << std::this_thread::get_id() << "]  async_send...\n";
	websocketpp::lib::error_code ec;
	m_pserver->send(hdl, msg, opcode, ec);
	if (ec)
	{
		std::cout << "Error sending message: " << ec.message() << std::endl;
	}
	return;
}

void websocket_server::on_close(websocketpp::connection_hdl hdl)
{
	websocketpp::lib::error_code ec;
	auto con = m_pserver->get_con_from_hdl(hdl,ec);
	if(ec)
	{
		std::cout << ec.message() << std::endl;
	}
	
	std::string client_ip_port = con->get_remote_endpoint();
	uint64_t hdl_addr = (uint64_t)std::addressof(*con.get());
	std::cout << "conn_addr [" << hdl_addr << "] " << " close connect: " << client_ip_port << std::endl;
	while(m_flag.test_and_set());
	m_connecter.erase(hdl_addr);
	m_flag.clear();
}

void websocket_server::on_open(websocketpp::connection_hdl hdl)
{
    Server::connection_ptr con = m_pserver->get_con_from_hdl(hdl);
	std::string client_ip_port = con->get_remote_endpoint();
	uint64_t hdl_addr = (uint64_t)std::addressof(*con.get());	
	std::cout << "conn_addr [" << hdl_addr << "] " << " open connect: " << client_ip_port << std::endl;
	while (m_flag.test_and_set());
	m_connecter.insert({hdl_addr,client_ip_port});
	m_flag.clear();
	
}

void websocket_server::on_message( websocketpp::connection_hdl hdl, Server::message_ptr msg)
{

	std::thread::id tid = std::this_thread::get_id(); 
	Server::connection_ptr con = m_pserver->get_con_from_hdl(hdl);
	uint64_t hdl_addr = (uint64_t)std::addressof(*con.get());
	std::cout 	<< "conn_addr [" << hdl_addr << "] " 
				<< "tid [" << tid << "] "<<"websocket on_message!" 
				<< std::endl;
	aaws_msg_sptr mst_ptr = std::make_shared<aaws_message>(msg);
	auto session = std::make_shared<aaws_session>(hdl,m_pserver,tid);
	if(0 != service_mgr::get_service_mgr().commit(session,mst_ptr))
	{
		std::cout << "service_mgr commit failed\n";
		return;
	}

}

bool websocket_server::on_ping(websocketpp::connection_hdl hdl, std::string payload)
{
    m_pserver->get_alog().write(websocketpp::log::alevel::app, payload);

	Server::connection_ptr con = m_pserver->get_con_from_hdl(hdl);
	std::string client_ip_port = con->get_remote_endpoint();

	std::cout << "recv ping: " << payload << " from " << client_ip_port << std::endl;
	try {
		m_pserver->pong(hdl, "pong");
	} catch (websocketpp::exception const &e) {
		std::cout << "error: " << e.what() << std::endl;
		return false;
	}
	return true;
}

bool websocket_server::on_pong(websocketpp::connection_hdl hdl, std::string payload)
{
    m_pserver->get_alog().write(websocketpp::log::alevel::app, payload);

	Server::connection_ptr con = m_pserver->get_con_from_hdl(hdl);
	std::string client_ip_port = con->get_remote_endpoint();

	std::cout << "recv pong from " << client_ip_port << ": " << payload << std::endl;
	return true;
}

bool websocket_server::on_validate(websocketpp::connection_hdl hdl)
{
    Server::connection_ptr con = m_pserver->get_con_from_hdl(hdl);
	std::string client_ip_port = con->get_remote_endpoint();
	std::string key = "Sec-WebSocket-Protocol";
	std::string token = con->get_request_header(key);
	std::cout << "token: " << token << std::endl;
	
	if (!verify_token(client_ip_port,token)) { // 不满足条件时直接返回FALSE，客户端会收到4类错误连接不上
		return false;
	}

	// 验证通过添加头部并返回true
	con->append_header(key, token); // 以后回复数据都要加这个头部信息
	return true;
}


bool websocket_server::verify_token(const std::string &connecter, const std::string &token)
{
	return true;
}