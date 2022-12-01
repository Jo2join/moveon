#include "aaws_message.h"

namespace DCS
{
    
std::string aaws_message::get_payload_method()
{
    std::string payload_method;
    int m = (int)(get_head()->payloadMethod);
    
    switch (m)
    {
    case 0:
        payload_method = "NONE";
        break;
    case 1:
        payload_method = "REQ";
        break;
    case 2:
        payload_method = "ACK";
        break;
    case 3:
        payload_method = "MESSAGE";
        break;
    case 4:
        payload_method = "CONFORM";
        break;
    default:
        break;
    }

    return payload_method;
}

} // namespace DCS

