
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>


typedef websocketpp::client<websocketpp::config::asio_client> client;
using namespace DCS;

class connection_metadata
{
public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
        : m_id(id), m_hdl(hdl), m_status("Connecting"), m_uri(uri), m_server("N/A")
    {
    }

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

    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg)
    {
        if (msg->get_opcode() == websocketpp::frame::opcode::text)
        {
            std::cout << msg->get_payload() << std::endl;
        }
        else
        {
            std::cout << "on binary message " << std::endl;
        }
    }

    void record_sent_message(std::string message)
    {
        m_messages.push_back(">> " + message);
    }

    std::string get_status()
    {
        return m_status;
    }

    websocketpp::connection_hdl &get_hdl()
    {
        return m_hdl;
    }

    int get_id()
    {
        return m_id;
    }

    friend std::ostream &operator<<(std::ostream &out, connection_metadata const &data);

private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
    std::vector<std::string> m_messages;
};

std::ostream &operator<<(std::ostream &out, connection_metadata const &data)
{
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason);
    out << "> Messages Processed: (" << data.m_messages.size() << ") \n";
    std::vector<std::string>::const_iterator it;
    for (it = data.m_messages.begin(); it != data.m_messages.end(); ++it)
    {
        out << *it << "\n";

        return out;
    }
    return out;
}

class websocket_endpoint
{
public:
    websocket_endpoint() : m_next_id(0)
    {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

        m_endpoint.init_asio();
        m_endpoint.start_perpetual();

        m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));
        avos::LoadAvosProto::load();//??????proto??????
    }

    ~websocket_endpoint()
    {
        m_endpoint.stop_perpetual();

        for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it)
        {
            if (it->second->get_status() != "Open")
            {
                // Only close open connections
                continue;
            }

            std::cout << "> Closing connection " << it->second->get_id() << std::endl;

            websocketpp::lib::error_code ec;
            m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
            if (ec)
            {
                std::cout << "> Error closing connection " << it->second->get_id() << ": "
                          << ec.message() << std::endl;
            }
        }

        m_thread->join();
    }

    int connect(std::string const &uri)
    {
        websocketpp::lib::error_code ec;

        client::connection_ptr con = m_endpoint.get_connection(uri, ec);

        if (ec)
        {
            std::cout << "> Connect initialization error: " << ec.message() << std::endl;
            return -1;
        }

        int new_id = m_next_id++;
        connection_metadata::ptr metadata_ptr(new connection_metadata(new_id, con->get_handle(), uri));
        m_connection_list[new_id] = metadata_ptr;

        con->set_open_handler(websocketpp::lib::bind(
            &connection_metadata::on_open,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1));
        con->set_fail_handler(websocketpp::lib::bind(
            &connection_metadata::on_fail,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1));
        con->set_message_handler(websocketpp::lib::bind(
            &connection_metadata::on_message,
            metadata_ptr,
            websocketpp::lib::placeholders::_1,
            websocketpp::lib::placeholders::_2));

        m_endpoint.connect(con);

        return new_id;
    }

    void close(int id, websocketpp::close::status::value code)
    {
        websocketpp::lib::error_code ec;

        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end())
        {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }

        m_endpoint.close(metadata_it->second->get_hdl(), code, "", ec);
        if (ec)
        {
            std::cout << "> Error initiating close: " << ec.message() << std::endl;
        }
    }

    void send(int id, std::string message, websocketpp::frame::opcode::value opcode = websocketpp::frame::opcode::text)
    {
        websocketpp::lib::error_code ec;

        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end())
        {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }

        m_endpoint.send(metadata_it->second->get_hdl(), message, opcode, ec);
        if (ec)
        {
            std::cout << "> Error sending message: " << ec.message() << std::endl;
            return;
        }

        metadata_it->second->record_sent_message(message);
    }

    connection_metadata::ptr get_metadata(int id) const
    {
        con_list::const_iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end())
        {
            return connection_metadata::ptr();
        }
        else
        {
            return metadata_it->second;
        }
    }
    void run()
    {
        m_endpoint.run();
    }
private:
    typedef std::map<int, connection_metadata::ptr> con_list;

    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    con_list m_connection_list;
    int m_next_id;
};

int main()
{
    bool done = false;
    std::string input;

    std::cout
    << "\nCommand List:\n"
    << "connect <ws uri>\n"
    << "show <connection id>\n"
    << "send <connection id> <message>\n"
    // << "aaws <connection id> <json-file>\n"
    << "show <connection id> <code>\n"
    << "help: Display this help text\n"
    << "quit: Exit the program\n"
    << std::endl;

    websocket_endpoint endpoint;
    int id = endpoint.connect("ws://localhost:6661");
    if (id != -1)
    {
        std::cout << "> Created connection with id " << id << std::endl;
    }

    while (!done)
    {
        std::cout << "Enter Command: ";
        std::getline(std::cin, input);

        if (input == "quit")
        {
            done = true;
            continue;
        }
        
        if (input == "help")
        {
            std::cout
                << "\nCommand List:\n"
                << "connect <ws uri>\n"
                << "show <connection id>\n"
                << "send <connection id> <message>\n"
                // << "aaws <connection id> <json-file>\n"
                << "show <connection id> <code>\n"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << std::endl;
        }
        else if (input.substr(0, 7) == "connect")
        {
            int id = endpoint.connect(input.substr(8));
            if (id != -1)
            {
                std::cout << "> Created connection with id " << id << std::endl;
            }
        }
        else if (input.substr(0, 4) == "show")
        {
            int id = atoi(input.substr(5).c_str());

            connection_metadata::ptr metadata = endpoint.get_metadata(id);
            if (metadata)
            {
                std::cout << *metadata << std::endl;
            }
            else
            {
                std::cout << "> Unknown connection id " << id << std::endl;
            }
        }
        else if (input.substr(0, 5) == "close")
        {
            std::stringstream ss(input);

            std::string cmd;
            int id;
            int close_code = websocketpp::close::status::normal;
            std::string reason;

            ss >> cmd >> id >> close_code;
            std::getline(ss, reason);

            endpoint.close(id, close_code);
        }
        else if (input.substr(0, 4) == "send")
        {
            std::stringstream ss(input);

            std::string cmd;
            int id;
            std::string message = "";

            ss >> cmd >> id;
            std::getline(ss, message);   
            endpoint.send(id, message);
        }
        // else if (input.substr(0, 4) == "aaws")
        // {
        //     std::stringstream ss(input);

        //     std::string cmd;
        //     std::string file;
        //     int id;
        //     ss >> cmd >> id >> file;
        //     aaws_message msg;
        //     JsonLoader loader;
        //     Json::Value root;
        //     if(0 != loader.DeserializationFromFile(file,root))
        //     {
        //         std::cout << file <<" read failed\n";
        //         continue;
        //     }
        //     std::string s = root.toStyledString();
        //     msg.append_option(aaws_message::opttype::opt_none,aaws_message::optformat::fmt_json,s);
        //     msg.submit(aaws_message::payload_method::REQ,1);
        //     AAWS_HEAD * head = (AAWS_HEAD * )msg.get_payload().data();
        //     AAWS_OPH *oph = (AAWS_OPH * )(msg.get_payload().data()+sizeof(AAWS_HEAD));
        //     std::string data(msg.get_payload().data()+sizeof(AAWS_HEAD)+sizeof(AAWS_OPH),oph->len);
        //     std::cout   << "option payload size: " << oph->len << std::endl
        //                 << "data: " << data << std::endl;
        //     endpoint.send(id, msg.get_payload(),websocketpp::frame::opcode::binary);
        // }
        else
        {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }

    return 0;
}
