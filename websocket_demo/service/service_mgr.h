#ifndef __SERVICE_MANAGER_H__
#define __SERVICE_MANAGER_H__

#include <string>
#include <map>
#include <memory>
#include "service_router.h"

namespace DCS
{
    class service_base;
    class aaws_message;
    class aaws_session;

    class service_mgr
    {
        typedef std::shared_ptr<aaws_message> aaws_msg_sptr;
        typedef std::shared_ptr<aaws_session> session_sptr;

    private:
        std::map<std::string,service_base *> _services;
        service_router _router;
        std::mutex _mutex;

    public:
        static service_mgr &get_service_mgr(){
            static service_mgr inst;
            return inst;
        }

        int regist(service_base *s);

        int unregist(const std::string &uri);

        int commit(session_sptr session, aaws_msg_sptr msg);

    private:
        service_base *get_service(const std::string &uri);
        service_mgr();
        ~service_mgr();
    };
    
} // namespace DCS


#endif