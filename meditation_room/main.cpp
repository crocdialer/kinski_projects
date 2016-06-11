#include "MeditationRoom.h"
#include "core/networking.hpp"

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::MeditationRoom>(argc, argv);
    LOG_INFO << "local ip: " << kinski::net::local_ip();
    return theApp->run();
}
