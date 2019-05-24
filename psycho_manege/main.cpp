#include "ModelViewer.h"

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::ModelViewer>(argc, argv);
    LOG_INFO << "local ip: " << crocore::net::local_ip();
    return theApp->run();
}
