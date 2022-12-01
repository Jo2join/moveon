#include <getopt.h>
#include <unistd.h>
#include <string>

const char *APP_NAME    = "compostdo";
const char *GIT_PATH    = "git@github.com:Jo2join/moveon.git";
const char *GIT_VERSION = "v0.1";

static const struct option long_options[] = {
    {"json-file",   1,  NULL,   'j'}, /* 必填选项 */
    {"directory",   0,  NULL,   'd'}, /* 选填选项 */
    {"help",        0,  NULL,   'h'},
    {"version",     0,  NULL,   'v'},
    {NULL,          0,  NULL,    0}
};

static void usage() {
    printf("codegen [option] [option [argument]] [...]\n");
    printf(" -d, --directory: specify home directory\n");
    printf(" -j, --json-file: specify json-file library to preload\n");
    printf(" -h, --help: help information\n");
    printf(" -v, --version: show app version info\n");
}

static void show_version() {
    printf("[AppName]    %s\n", APP_NAME);
    printf("[GitPath]    %s\n", GIT_PATH);
    printf("[GitVersion] %s\n", GIT_VERSION);
    printf("[BuildData]  %s\n", __DATE__);
    printf("[BuildTime]  %s\n", __TIME__);
}

struct CmdOption
{
    std::string jfile;
    std::string dir{"./"};
};      

int main(int argc, char *argv[]) {
    int opt = 0, index = 0;
    CmdOption cmdop;
    while ((opt = getopt_long(argc, argv, "d:j:hv", long_options, &index)) != -1) {
        switch (opt) {
        case 'd':
            cmdop.dir = optarg;
            break;
        case 'j':
            cmdop.jfile = optarg;
            break;
        case 'v':
            show_version();
            return 0;
        case 'h':  // help
        case '?':
        default:
            usage();
            return -1;
        }
    }

    if (cmdop.jfile.empty()) {
        printf("ERROR: no target file specified, use -j instead\n");
        usage();
        return -2;
    }

	/* 判断组件根目录是否存在 */
    if (0 != chdir(cmdop.dir.c_str()))
    {
        printf("ERROR: can not access [%s]\n",cmdop.dir.c_str());
        return -1;
    }

    //do something
    printf("tool cmd excute success\n");

	return 0;
}
