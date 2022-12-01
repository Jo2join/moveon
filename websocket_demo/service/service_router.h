#ifndef _SERVICE_ROUTER_H__
#define _SERVICE_ROUTER_H__

#include <string>
#include <memory>
#include "aaws_message.h"

namespace DCS
{
    class service_router
    {
        typedef std::shared_ptr<aaws_message> aaws_msg_sptr;

    private:
    public:
        service_router(){};

        ~service_router(){};
        
        /// @brief 获取服务的uri，用于服务路由
        /// 如果option包含多个则将uri信息放在扩展头里面
        /// 如果option只有一个则将uri放在option里面
        /// @param msg ws服务消息
        /// @return 服务uri
        std::string get_uri(aaws_msg_sptr msg);
    };

} // namespace DCS

#endif