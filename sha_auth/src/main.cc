// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <iostream>
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <coordinator_client.h>
#include "server.h"
#include "logging_common.h"
#include "service.pb.h"
#include "auth_service.h"
#include "getifaddr.h"
#include "consts.h"

namespace po = boost::program_options;
int
main (int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    // Initialize logging
    init_logging();

    // Option processing
    po::options_description desc("Auth service options");
    std::string saddr = "0.0.0.0", 
                sport = "8080", 
                servicename = "my_service";

    std::string coordinator = "127.0.0.1";

    // Coordinator port
    int32_t cport = hlv::service::lookup::UPDATE_PORT;

    // Flags for auth server
    desc.add_options()
        ("help,h", "Display help")
        ("addr,a", po::value<std::string>(&saddr), "Service address")
        ("port,p", po::value<std::string>(&sport)->implicit_value("8080"), "Service port")
        ("name,n", po::value<std::string>(&servicename)->implicit_value(servicename), "Service name")
        ("coordinator,c", po::value<std::string>(&coordinator)->implicit_value(coordinator), "Coordinator")
        ("cport,cp", po::value<int32_t>(&cport)->implicit_value(cport), "Coordinator port");
    po::options_description options;
    options.add(desc);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cerr << desc;
        return 0;
    }

    // Create coordinator client
    hlv::coordinator::EvUpdateClient cclient (coordinator, cport);
    hlv::coordinator::EvUpdateClient::TypeValueMap vmap;
    std::string my_address = saddr;
    bool success;

    // Figure out our IP address if bound to all
    if (my_address == "0.0.0.0") {
        success = getFirstNonLoopbackAddress (my_address);
        if (!success) {
            std::cerr << "Failed to find address" << std::endl;
            return 1;
        }
    }

    // Concatenate address and port
    std::stringstream full_address;
    full_address << my_address << ":" << sport;

    // Insert into the lookup DB
    vmap.insert(std::make_pair(hlv::service::lookup::AUTH_LOCATION, full_address.str()));
    success = cclient.connect ();
    if (!success) {
        std::cerr << "Failed to connect to coordinator" << std::endl;
    }
    success = cclient.set_values (0, servicename, vmap); 
    if (!success) {
        std::cerr << "Failed to register auth service" << std::endl;
    }
    cclient.disconnect ();

    // Start server
    boost::asio::io_service io_service;
    hlv::service::server::Server server (
            io_service,
            saddr, 
            sport, 
            std::make_shared<hlv::service::server::AuthService>());
    server.start();

    // Register to quit when necessary
    boost::asio::signal_set signals (io_service);
    signals.add (SIGINT);
    signals.add (SIGTERM);
#if defined(SIGQUIT)
    signals.add (SIGQUIT);
#endif
    signals.async_wait ([&](boost::system::error_code, int) {
        std::cout << "Quitting" << std::endl;
        server.stop ();
        io_service.stop ();
    });
    // This thread now provides I/O service
    io_service.run();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
