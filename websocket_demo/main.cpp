#include "websocket_server.h"
// #include "settings.h"
#include <iostream>
#include <signal.h>
#include <thread>
#include "thread_pool.h"
// #include "service_echo.h"
// #include "service_plugin.h"

using namespace DCS;

static int g_exit = 0;

void sig_call_back(int sig)
{
    printf("siginit catch\n");
    if (sig == SIGINT)
    {
        g_exit = 1;
    }
}

int main()
{

    signal(SIGINT, sig_call_back);
    
    // std::string work_dir = getenv("WORK_DIR");
    // if (work_dir.empty())
    // {
    //     std::cout << " please set WORK_DIR dir!" << std::endl;
    // }

    /*config file init*/
    // websocket_config &dis_config = websocket_config::get_instance();
    // dis_config.init(work_dir + "/websocket_service/cfg/config.yaml");

    /* init thread pool */
    thread_pool_global_intance::intance().init_thread_pool();

    /*websocket server start*/
    websocket_server::instance().init_service();
    websocket_server::instance().run();

    /* service start */
    
    // service_echo ec;
    // ec.start();
    
    // LoadServicePlugin load(work_dir + "/websocket_service/cfg/test.json");

    while (!g_exit)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    websocket_server::instance().exit();
    // websocket_server::instance().wait();
    return 0;
}
