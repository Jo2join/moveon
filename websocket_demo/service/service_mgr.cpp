#include <iostream>
#include "service_mgr.h"
#include "service_base.h"
#include "aaws_message.h"
#include "thread_pool.h"
#include "aaws_session.h"

namespace DCS
{
    service_mgr::service_mgr()
    {
    }
    
    service_mgr::~service_mgr()
    {
    }

    int service_mgr::regist(service_base *s)
    {
        _services[s->uri()] = s;
        return 0;
    }
    
    int service_mgr::unregist(const std::string &uri)
    {
        auto iter = _services.find(uri);
        if(_services.end() == iter)
        {
            _services.erase(uri);
        }
        return 0;
    }

    int service_mgr::commit(session_sptr session, aaws_msg_sptr msg_ptr)
    {
        std::string uri = _router.get_uri(msg_ptr);
        service_base *ps = get_service(uri);
        if(nullptr == ps)
        {
            return -1;
        }
        thread_pool_global_intance::intance().add_task(thread_pool::normal_task,&service_base::doService,ps,session,msg_ptr);
        return 0;
    }

    service_base *service_mgr::get_service(const std::string &uri)
    {
        std::unique_lock<std::mutex> lc(_mutex);
        auto iter = _services.find(uri);
        if(_services.end() == iter)
        {
            return nullptr;
        }
        return iter->second;
    }

} // namespace DCS