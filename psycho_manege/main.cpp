#include "ModelViewer.h"
#include "core/networking.hpp"

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<kinski::ModelViewer>(argc, argv);
    LOG_INFO << "local ip: " << kinski::net::local_ip();
    theApp->set_window_title(kinski::get_filename_part(argv[0]));
    return theApp->run();
}