#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include "server.h"
#include "logging_common.h"
#include "service.pb.h"
#include "simple_service.h"

int
main (int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    init_logging();
    if (argc != 3) {
        std::cerr << "Usage: hlv_service <address> <port>\n";
        return 1;
    }
    boost::asio::io_service io_service;
    hlv::service::server::Server server (
            io_service,
            argv[1], 
            argv[2], 
            std::make_shared<hlv::service::server::SimpleService>());
    server.start();
    // This thread now provides I/O service
    io_service.run();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;

}
