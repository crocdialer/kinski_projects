#include "GrowthApp.h"
#include "crocore/networking.hpp"

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::GrowthApp>(argc, argv);
    LOG_INFO << "local ip: " << crocore::net::local_ip();
    return theApp->run();
}
