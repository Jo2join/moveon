
#ifndef __AAWS_MESSAGE_H__
#define __AAWS_MESSAGE_H__
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>
#include <memory>
#include <websocketpp/server.hpp>
#include <websocketpp/transport/asio/endpoint.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include "jsoncpp/json/json.h"

namespace DCS{

struct AAWS_HEAD
{
    uint8_t magic[4];
    uint32_t version;
    uint32_t payloadLen;
    uint8_t payloadMethod;
    uint8_t exhcnt;
    uint8_t optcnt;
    uint8_t hdrsv1;
    uint32_t exhlen;
    uint32_t optlen;
    uint32_t exhcrc32;
    uint32_t optcrc32;
    uint32_t seqence;
    uint8_t hdrsv[4];
    std::string dump_msg()
    {
        std::stringstream ss;
        ss << "----AAWS_HEAD----\n";
        ss << "magic: "          << magic[0]<< magic[1]<< magic[2]<< magic[3] << "\n";
        ss << "version: "        << (uint32_t)version << "\n";
        ss << "payloadLen: "     << (uint32_t)payloadLen <<"\n"; 
        ss << "payloadMethod: "  << (uint32_t)payloadMethod << "\n";
        ss << "exhcnt: "         << (uint32_t)exhcnt << "\n";
        ss << "optcnt: "         << (uint32_t)optcnt << "\n";
        ss << "hdrsv1: "         << (uint32_t)hdrsv1 << "\n";
        ss << "exhlen: "         << (uint32_t)exhlen << "\n";
        ss << "optlen: "         << (uint32_t)optlen << "\n";
        ss << "exhcrc32: "     << (uint32_t)exhcrc32 << "\n";
        ss << "optcrc32: "     << (uint32_t)optcrc32 << "\n";
        ss << "seqence: "         << seqence << "\n";
        return ss.str();
    }
    AAWS_HEAD()
    {
        magic[0] = 'A';
        magic[1] = 'D';
        magic[2] = 'A';
        magic[3] = 'S';
        version = 0;
        payloadLen = 0;
        payloadMethod = 0;
        exhcnt = 0;
        optcnt = 0;
        hdrsv1 = 0;
        exhlen = 0;
        optlen = 0;
        exhcrc32 = 0;
        optcrc32 = 0;
        seqence = 0;
        hdrsv[0] = 0;
        hdrsv[1] = 0;
        hdrsv[2] = 0;
        hdrsv[3] = 0;
    }
};

struct AAWS_EXH
{
    uint8_t type;
    uint16_t len;
    uint8_t rsv[5];
    std::string dump_msg()
    {
        std::stringstream ss;
        ss << "----AAWS_EXH----\n";
        ss << "type: "    << (uint32_t)type << "\n";
        ss << "len: "     << (uint32_t)len <<"\n";
        ss << "rsv: "     << (uint16_t)rsv[0]<< (uint16_t)rsv[1]<< (uint16_t)rsv[2]<< (uint16_t)rsv[3] << (uint16_t)rsv[4] << "\n";
        return ss.str();
    }

    AAWS_EXH()
    {
        type = 0;
        len = 0;
        rsv[0] = 0;
        rsv[1] = 0;
        rsv[2] = 0;
        rsv[3] = 0;
        rsv[4] = 0;
    }
};

struct AAWS_OPH
{
    uint8_t type;
    uint8_t format;
    uint8_t rsv[2];
    uint32_t len;

    std::string dump_msg()
    {
        std::stringstream ss;
        ss << "----AAWS_OPH----\n";
        ss << "type: "    << (uint32_t)type << "\n";
        ss << "format: "  << (uint32_t)format <<"\n";
        ss << "rsv: "     << (uint16_t)rsv[0]<< (uint16_t)rsv[1]<< "\n";
        ss << "len: "     << (uint32_t)len <<"\n";
        return ss.str();
    }

    AAWS_OPH()
    {
        type = 0;
        format = 0;
        len = 0;
        rsv[0] = 0;
        rsv[1] = 0;
    }
};

class aaws_message
{

typedef websocketpp::server<websocketpp::config::asio>::message_ptr websocket_msg_ptr;

private:
    AAWS_HEAD _header;//????????????????????????????????????
    std::vector<AAWS_EXH> _exheaders;//????????????????????????????????????
    // std::vector<std::string> _exhdata;//????????????????????????????????????
    std::vector<AAWS_OPH> _opt_headers;//????????????????????????????????????
    std::string _payload{"\0",sizeof(AAWS_HEAD)};
    bool _use_ex_msg_ptr;
    websocket_msg_ptr _ptr;//??????????????? ????????????message_ptr?????????????????????
    
public:
    typedef enum payload_method
    {
        NONE = 0,
        REQ = 1,
        ACK = 2,
        MESSAGE = 3,
        CONFIRM = 4
    }payload_method;

    typedef enum exhtype
    {
        eth_none = 0

    }exhtype;

    typedef enum optformat
    {
        fmt_none = 0,
        fmt_text = 1,
        fmt_binary = 2,
        fmt_json = 3,
        fmt_protobuf = 4
    }optformat;

    typedef enum opttype
    {
        opt_none = 0
    }opttype;

    /// @brief ??????????????????????????????????????????
    aaws_message():_use_ex_msg_ptr(false){};

    /// @brief ??????????????????????????????????????????
    /// @param msg websocketpp???????????????????????????????????????
    aaws_message(websocket_msg_ptr msg)
    :_use_ex_msg_ptr(true)
    ,_ptr(msg)
    {};

    // aaws_message(const aaws_message &o);
    // aaws_message & operator=(const aaws_message &o);
    ~aaws_message(){};

    std::string get_payload_method();

    /// @brief ??????ws???????????????option??????
    /// @param t option?????? ?????????????????????
    /// @param fmt option???????????? json text binary???
    /// @param o option?????? ?????????8???????????????
    /// @return ?????????????????????
    inline aaws_message &append_option(opttype t, optformat fmt, const std::string &o)
    {
        AAWS_OPH opt;
        opt.format = (int)fmt;
        opt.type = (int)t;
        opt.len = o.size();
        _payload += std::string((const char *)(&opt),sizeof(AAWS_OPH));
        _payload += o;
        // _opt_headers.emplace_back(opt);

        _header.optcnt++;
        // _header.optcrc32
        _header.optlen += o.size() + sizeof(AAWS_OPH);

        return *this;
    }

    /// @brief ??????ws???????????????????????????
    /// @param t ??????????????? ????????????
    /// @param h ?????????????????? ?????????8???????????????
    /// @return ?????????????????????
    inline aaws_message &append_exh(exhtype t, const std::string &h)
    {
        AAWS_EXH exh;
        exh.type = t;
        exh.len = h.size();
        _payload += std::string((const char *)(&exh),sizeof(AAWS_EXH));
        _payload += h;

        _header.exhcnt++;
        // _header.exhcrc32
        _header.exhlen+=h.size() + sizeof(AAWS_EXH);
        
        return *this;
    }

    /// @brief ????????????????????????????????????????????????????????????
    /// @param m ??????????????? NONE REQ ACK MESSAGE CONFORM
    /// @param sequence ?????????
    /// @return ?????????????????????
    inline aaws_message &submit(payload_method m, uint32_t sequence = 0)
    {
        _header.payloadMethod = m;
        _header.seqence = sequence;
        ///??????????????????????????????????????????
        _header.payloadLen = sizeof(AAWS_HEAD) + _header.exhlen + _header.optlen;
        ///?????????????????????
        void *p = (void *)(_payload.data());
        memcpy(p,(void*)(&_header),sizeof(AAWS_HEAD));
        return *this;
    }

    /// @brief ??????payload??????
    /// @return payload???????????????
    inline std::string const &get_payload()
    {
        if (_use_ex_msg_ptr)
        {
            return _ptr->get_payload();
        }
        else
        {
            return _payload;
        }
    }

    /// @brief ??????payload???????????????
    /// @return payload????????????
    inline size_t size() { return get_head()->payloadLen; }

    /// @brief ?????????????????????
    /// @return ???????????????
    inline uint32_t sequence() { return get_head()->seqence; }

    /// @brief ?????????????????????
    /// @return ?????????????????????
    inline uint8_t get_exh_cnt(){return get_head()->exhcnt;}

    /// @brief ??????option????????????
    /// @return ??????option??????
    inline uint8_t get_opt_cnt(){return get_head()->optcnt;}

    /// @brief ???????????????????????????????????????
    /// @return ???????????????????????????????????????
    inline uint32_t get_exh_size(){return get_head()->exhlen;}

    /// @brief ????????????option??????????????????
    /// @return ????????????option??????????????????
    inline uint32_t get_opt_size(){return get_head()->optlen;}

    /// @brief ??????ws?????????option
    /// @param opts ws?????????option?????????????????????8??????option????????????option?????????
    /// @return ????????? 0????????? ???????????????
    inline int get_option(std::vector<std::string> &opts)
    {
        std::string optdata;
        AAWS_HEAD *ph = get_head();
        uint32_t offset = sizeof(AAWS_HEAD) + ph->exhlen;
        for(int i = 0; i < ph->optcnt; i++)
        {
            AAWS_OPH *optH = (AAWS_OPH *)((const char *)ph + offset);
            uint32_t optlen = optH->len + sizeof(AAWS_OPH);
            std::string s((const char *)optH, optlen);
            opts.emplace_back(s);
            offset += optlen;
        }     
        return 0;
    }

    /// @brief ??????ws???????????????id???option
    /// @param s ws?????????option???????????????8??????option????????????option?????????
    /// @param id option??????
    /// @return ????????? 0????????? ???????????????
    inline int get_option(std::string &s,int id = 0)
    {
        if(id >= get_opt_cnt() || id < 0)
        {
            return -1;
        }
        AAWS_HEAD *ph = get_head();
        uint32_t offset = sizeof(AAWS_HEAD) + ph->exhlen;
        AAWS_OPH *optH = (AAWS_OPH *)((const char *)ph + offset);
        for(int i = 0; i < id; i++)
        {
            offset += optH->len + sizeof(AAWS_OPH);
            optH = (AAWS_OPH *)(ph + offset);
        } 
        std::string opt((const char *)ph + offset, optH->len + sizeof(AAWS_OPH));
        s.swap(opt);
        return 0;
    }

    /// @brief ??????ws??????????????????????????????
    /// @param exs ws???????????????????????????????????????8??????????????????????????????
    /// @return ????????? 0????????? ???????????????
    inline int get_exh(std::vector<std::string> &exs)
    {
        std::string exh;
        AAWS_HEAD *ph = get_head();
        uint32_t offset = sizeof(AAWS_HEAD);
        for(int i = 0; i < ph->exhcnt; i++)
        {
            AAWS_EXH *exhead = (AAWS_EXH *)((const char *)ph + offset);
            auto len = exhead->len + sizeof(AAWS_EXH);
            std::string s((const char *)exhead , len);
            exs.emplace_back(s);
            offset += len;
        }
        return 0;
    }

    /// @brief ??????ws???????????????id??????????????????
    /// @param s ws?????????????????????????????????8??????????????????????????????
    /// @param id ???????????????
    /// @return ????????? 0????????? ???????????????
    inline int get_exh(std::string &s, int id)
    {
        if(id >= get_exh_cnt() || id < 0)
        {
            return -1;
        }
        AAWS_HEAD *ph = get_head();
        uint32_t offset = sizeof(AAWS_HEAD);
        AAWS_EXH *exhead = (AAWS_EXH *)((const char *)ph + offset);
        for(int i = 0; i < id; i++)
        {
            offset += exhead->len + sizeof(AAWS_EXH);
            exhead = (AAWS_EXH *)((const char *)ph + offset);
        }
        std::string exh((const char *)exhead, exhead->len + sizeof(AAWS_EXH));
        s.swap(exh);
        return 0;
    }

    /// @brief ??????ws??????????????????????????????
    /// @param exs ws??????????????????????????????????????????8???????????????
    /// @return ????????? 0????????? ???????????????
    inline int get_exh_payload(std::vector<std::string> &exs)
    {
        std::string exh;
        AAWS_HEAD *ph = get_head();
        uint32_t offset = sizeof(AAWS_HEAD);
        for(int i = 0; i < ph->exhcnt; i++)
        {
            AAWS_EXH *exhead = (AAWS_EXH *)((const char *)ph + offset);
            auto len = exhead->len + sizeof(AAWS_EXH);
            std::string s((const char *)exhead + sizeof(AAWS_EXH), exhead->len);
            exs.emplace_back(s);
            offset += len;
        }
        return 0;
    }

    /// @brief ??????ws???????????????id??????????????????
    /// @param s ws????????????????????????????????????8???????????????
    /// @param id ???????????????
    /// @return ????????? 0????????? ???????????????
    inline int get_exh_payload(std::string &s, int id)
    {
        if(id >= get_exh_cnt() || id < 0)
        {
            return -1;
        }
        AAWS_HEAD *ph = get_head();
        uint32_t offset = sizeof(AAWS_HEAD);
        AAWS_EXH *exhead = (AAWS_EXH *)((const char *)ph + offset);
        for(int i = 0; i < id; i++)
        {
            offset += exhead->len + sizeof(AAWS_EXH);
            exhead = (AAWS_EXH *)((const char *)ph + offset);
        }
        std::string exh((const char *)exhead + sizeof(AAWS_EXH), exhead->len);
        s.swap(exh);
        return 0;
    }

    /// @brief ??????ws??????????????????????????????
    /// @param exs ws??????????????????????????????????????????8???????????????
    /// @return ????????? 0????????? ???????????????
    inline std::vector<AAWS_EXH> get_exh_8bit()
    {
        std::vector<AAWS_EXH> exhs;
        AAWS_HEAD *ph = get_head();
        uint32_t offset = sizeof(AAWS_HEAD);
        for(int i = 0; i < ph->exhcnt; i++)
        {
            AAWS_EXH exhead = *(AAWS_EXH *)((const char *)ph + offset);
            offset += exhead.len + sizeof(AAWS_EXH);
            exhs.emplace_back(exhead);
        }
        return exhs;
    }

    /// @brief ??????ws???????????????id??????????????????
    /// @param id ???????????????
    /// @return ws????????????????????????????????????8???????????????
    inline int get_exh_8bit(AAWS_EXH &exh, int id)
    {
        if(id >= get_exh_cnt() || id < 0)
        {
            return -1;
        }
        AAWS_HEAD *ph = get_head();
        uint32_t offset = sizeof(AAWS_HEAD);
        for(int i = 0; i < id; i++)
        {
            AAWS_EXH *exhead = (AAWS_EXH *)((const char *)ph + offset);
            offset += exhead->len + sizeof(AAWS_EXH);
        }
        exh = *(AAWS_EXH*)((const char *)ph + offset);
        return 0;
    }

    inline void dump_msg()
    {
        std::cout << "\n--------------dump_msg--------------\n\n";
        std::cout << get_head()->dump_msg();
        std::string option;
        get_option(option);
        auto oph = (AAWS_OPH*)(option.data());
        std::cout << oph->dump_msg();
        std::cout << "\n--------------option--------------\n\n";
        std::cout << std::string(option.data()+sizeof(AAWS_OPH),oph->len) << "\n";
        std::cout << "\n--------------dump_msg end--------------\n\n";
    }

private:
    inline AAWS_HEAD *get_head()
    {
        AAWS_HEAD *ph = nullptr;
        if(_use_ex_msg_ptr)
        {
            ph = (AAWS_HEAD *)(_ptr->get_payload().data());
        }
        else
        {
            ph = &_header;
        }
        return ph;
    }

};

typedef std::shared_ptr<aaws_message> aaws_msg_sptr;
typedef std::weak_ptr<aaws_message> aaws_msg_wptr;

} // namespace DCS


#endif