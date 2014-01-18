// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
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
#include <hiredisasio.h>
#include "consts.h"
#include "logging_common.h"
#include "coordinator_server.h"

// Main file for EV lookup coordinator
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
    po::options_description desc("Update service options");
    std::string address = "0.0.0.0",
                port    = std::to_string (hlv::service::lookup::UPDATE_PORT),
                redisAddress = "127.0.0.1",
                prefix = hlv::service::lookup::REDIS_PREFIX;
    int32_t redisPort = hlv::service::lookup::REDIS_PORT;
    desc.add_options()
        ("help,h", "Display help")
        ("address,a", po::value<std::string>(&address)->implicit_value(address), "Bind to address")
        ("port", po::value<std::string>(&port)->implicit_value(port), "Bind to port")
        ("raddress,r", po::value<std::string>(&redisAddress)->implicit_value(redisAddress), "Redis server")
        ("rport", po::value<int32_t>(&redisPort)->implicit_value(redisPort), "Redis port")
        ("prefix,p", po::value<std::string>(&prefix), 
                   "Prefix for redis DB");
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

    if (!vm.count ("prefix")) {
        std::cerr << "Cowardly failing to start without prefix (tenant ID)" << std::endl;
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

    redisAsyncSetConnectCallback (context, connectCallback);
    redisAsyncSetDisconnectCallback (context, disconnectCallback);

    // Create server
    boost::asio::io_service io_service;

    asio_redis::redisBoostClient client (io_service, context);
    // Server information
    hlv::service::coordinator::ConnectionInformation information 
                                                    (redisAddress,
                                                     redisPort,
                                                     context,
                                                     prefix);
    // Create an update server
    hlv::service::coordinator::Server update (
            io_service,
            address,
            port,
            information);
    BOOST_LOG_TRIVIAL(info) << "Starting update server" << std::endl;
    update.start ();

    boost::asio::signal_set signals (io_service);
    signals.add (SIGINT);
    signals.add (SIGTERM);
#if defined(SIGQUIT)
    signals.add (SIGQUIT);
#endif
    signals.async_wait ([&](boost::system::error_code, int) {
        std::cout << "Quitting" << std::endl;
        update.stop ();
        client.stop ();
        io_service.stop ();
    });
    // This thread now provides I/O service
    io_service.run();
    redisAsyncDisconnect (context);
    google::protobuf::ShutdownProtobufLibrary();
    return 0;

}
