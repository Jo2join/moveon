#include <iostream>
#include "service_router.h"

namespace DCS
{
    std::string service_router::get_uri(aaws_msg_sptr msg)
    {
        std::string uri;
        msg->dump_msg();
        if (msg->get_exh_cnt())
        {
            std::string exheadPayload;
            msg->get_exh_payload(exheadPayload, 0);
            std::cout << exheadPayload << std::endl;
            Json::Reader reader;
            Json::Value root;
            if (false == reader.parse(exheadPayload, root))
            {
                return uri;
            }
            if (root.isMember("service_uri"))
            {
                uri = root["service_uri"].asString();
            }
            return uri;
        }

        std::vector<std::string> opts;
        msg->get_option(opts);
        for (auto &opt : opts)
        {
            AAWS_OPH *oph = ((AAWS_OPH *)opt.data());
            std::cout << std::string(opt.data() + sizeof(AAWS_OPH),oph->len);
            if (oph->format != aaws_message::optformat::fmt_json)
            {
                continue;
            }

            Json::Reader reader;
            Json::Value root;
            if (false == reader.parse(opt.data() + sizeof(AAWS_OPH), opt.data() + oph->len + sizeof(AAWS_OPH), root))
            {
                return uri;
            }
            if (root.isMember("service_uri"))
            {
                uri = root["service_uri"].asString();
            }
            return uri;
        }
        return uri;
    }
}//namespace DCS