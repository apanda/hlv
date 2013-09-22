// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <iostream>
#include <memory>
#include <thread>
#include <tuple>
#include <sstream>
#include <signal.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <getifaddr.h>
#include <update_client.h>
#include "consts.h"
#include "logging_common.h"
#include "rendezvous_server.h"

// Main file for EV ebox server
namespace po = boost::program_options;

int
main (int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    init_logging();

    // Argument parsing
    po::options_description desc("Update service options");
    std::string address = "0.0.0.0",
                port    = std::to_string (hlv::service::lookup::EBOX_RENDEZVOUS_PORT),
                name    = "my_service",
                coordinator = "127.0.0.1";
    uint32_t coordinator_port = hlv::service::lookup::UPDATE_PORT;
    uint64_t accessibleBy = 0;
    desc.add_options()
        ("help,h", "Display help")
        ("address,a", po::value<std::string>(&address)->implicit_value(address), "Bind to address")
        ("port,p", po::value<std::string>(&port)->implicit_value(port), "Bind to port")
        ("coordinator,c", po::value<std::string>(&coordinator)->implicit_value (coordinator),
            "Coordinator address")
        ("cport", po::value<uint32_t>(&coordinator_port)->implicit_value (coordinator_port),
            "Coordinator port")
        ("name,n", po::value<std::string>(&name)->implicit_value (name),
            "Service name")
        ("token,t", po::value<uint64_t>(&accessibleBy)->implicit_value (accessibleBy),
            "Token for access");
    po::options_description options;
    options.add(desc);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).run(), vm);
    po::notify(vm);

    if (vm.count ("help")) {
        std::cerr << desc << std::endl;
        return 0;
    }

    // Create server
    boost::asio::io_service io_service;

    // Server information
    hlv::service::ebox::rendezvous::SessionMap map;
    hlv::service::ebox::rendezvous::ConnectionInformation information 
                                                    (map);

    // Create an rendezvous server
    hlv::service::ebox::rendezvous::Server rendezvous (
            io_service,
            address,
            port,
            information);
    rendezvous.start ();

    // Cannonical address
    if (address == "0.0.0.0") {
        bool asuccess = getFirstNonLoopbackAddress (address);
        if (asuccess) {
            std::cout << "Got IP address " << address << std::endl;;
        } else {
            std::cerr << "Failed to find IP " << std::endl;
            return 0;
        }
    }
    std::stringstream addressStr;
    addressStr << address << ":" << port;
    std::string regAddress = addressStr.str ();

    // Register
    hlv::lookup::update::EvUpdateClient coordClient (coordinator,
                                         coordinator_port);
    coordClient.connect ();
    std::map<std::string, std::string> changes = {{hlv::service::lookup::RENDEZVOUS_LOCATION, 
                                                   regAddress}};
    bool succ = coordClient.set_values (accessibleBy, name, changes);
    if (!succ) {
        std::cerr << "Failed to register" << std::endl;
        return 0;
    }
    succ = coordClient.set_permissions (accessibleBy, name, accessibleBy);
    if (!succ) {
        std::cerr << "Failed to set permissions" << std::endl;
        return 0;
    }

    boost::asio::signal_set signals (io_service);
    signals.add (SIGINT);
    signals.add (SIGTERM);
#if defined(SIGQUIT)
    signals.add (SIGQUIT);
#endif
    signals.async_wait ([&](boost::system::error_code, int) {
        std::cout << "Quitting" << std::endl;
        rendezvous.stop ();
        io_service.stop ();
    });
    // This thread now provides I/O service
    io_service.run();
    google::protobuf::ShutdownProtobufLibrary();
    return 1;
}
