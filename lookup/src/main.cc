#include <iostream>
#include <memory>
#include <thread>
#include <tuple>
#include <signal.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include "server.h"
#include "logging_common.h"
#include "lookup_server.h"
#include "hiredisasio.h"

namespace po = boost::program_options;
namespace {
void connectCallback (const redisAsyncContext* c, int status) {
    if (status != REDIS_OK) {
        std::cerr << "Error " << c->errstr << std::endl;
        return;
    }
    BOOST_LOG_TRIVIAL (info) << "Connected with redis";
}

void disconnectCallback (const redisAsyncContext* c, int status) {
    BOOST_LOG_TRIVIAL (info) << "Disconnected ";
    if (status != REDIS_OK) {
        std::cerr << "Disconnected due to error " << c->errstr << std::endl;
    }
}
}

int
main (int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    init_logging();

    // Argument parsing
    po::options_description desc("Lookup service options");
    std::string address = "0.0.0.0",
                port    = "8085",
                redisAddress = "127.0.0.1";
    int32_t redisPort = 6379;
    desc.add_options()
        ("help,h", "Display help")
        ("address,a", po::value<std::string>(&address)->implicit_value("0.0.0.0"), "Bind to address")
        ("port,p", po::value<std::string>(&port)->implicit_value("8085"), "Bind to port")
        ("raddress,r", po::value<std::string>(&redisAddress)->implicit_value("127.0.0.1"), "Redis server")
        ("rport", po::value<int32_t>(&redisPort)->implicit_value(6379), "Redis port");
    po::options_description options;
    options.add(desc);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cerr << desc << std::endl;
        return 0;
    }

    // Connect to Redis
    redisAsyncContext *context = redisAsyncConnect (redisAddress.c_str(), redisPort);
    if (!context) {
        std::cerr << "Failed to allocate redis context" << std::endl;
        return 0;
    }

    if (context->err) {
        std::cerr << "Failed to connect to redis instance " << context->errstr << std::endl;
        return 0;
    }  

    // Create server
    boost::asio::io_service io_service;

    asio_redis::redisBoostClient client (io_service, context);
    // Server information
    hlv::service::lookup::ConnectionInformation information 
                                                    ("authed", 
                                                     redisAddress,
                                                     redisPort,
                                                     context);
    // Create a lookup server            
    hlv::service::lookup::Server lookup (
            io_service,
            address,
            port,
            information);
    lookup.start ();
    redisAsyncSetConnectCallback (context, connectCallback);
    redisAsyncSetDisconnectCallback (context, disconnectCallback);
    // This thread now provides I/O service
    io_service.run();
    redisAsyncDisconnect (context);
    google::protobuf::ShutdownProtobufLibrary();
    return 0;

}
