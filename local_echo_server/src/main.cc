// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <iostream>
#include <string>
#include <list>
#include <thread>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <coordinator_client.h>
#include <logging_common.h>
#include <query_client.h>
#include <update_client.h>
#include <echo_server.h>
#include "consts.h"
#include "getifaddr.h"

namespace po = boost::program_options;

void launchService (boost::asio::io_service& io_service,
                   hlv::service::echo::server::Server& server) {
    server.start ();
}

std::unique_ptr<hlv::ebox::update::EvLDiscoveryClient>
          discoverEdgeBox (hlv::lookup::client::EvLookupClient& client,
                                                       uint64_t token) {
    bool result;
    std::list<std::string> results;
    uint64_t resultToken;
    result = client.LocalQuery (token, 
                        hlv::service::lookup::LDEBOX_LOCATION,
                        resultToken,
                        results);
    if (!result || results.size () == 0) {
        result = 0;
        return nullptr;
    }
    const std::string location = results.front ();
    std::vector<std::string> split_results;
    boost::split(split_results, location, boost::is_any_of(":"), boost::token_compress_off);
    if (split_results.size () != 2) {
        result = false;
        std::cerr << "Do not understand response" << std::endl;
        return nullptr;
    }
    auto ret = std::unique_ptr<hlv::ebox::update::EvLDiscoveryClient>(
                    new hlv::ebox::update::EvLDiscoveryClient (token,
                                                               split_results[0],
                                                               std::stoul (split_results[1])));
    return ret;
}

int main (int argc, char* argv[]) {
    init_logging();
    std::string address = "0.0.0.0",
                name = "my_service",
                coordinator = "127.0.0.1",
                type = hlv::service::lookup::ECHO_LOCATION;
    uint32_t coordinator_port = hlv::service::lookup::SERVER_PORT,
              port = 8000;
    uint64_t accessibleBy = 0;
    po::options_description desc("Simple Server");
    desc.add_options()
        ("help,h", "Display help") 
        ("ldiscovery,c", po::value<std::string>(&coordinator)->implicit_value (coordinator),
            "Local discovery address")
        ("lport", po::value<uint32_t>(&coordinator_port)->implicit_value (coordinator_port),
            "Local discovery port")
        ("address", po::value<std::string>(&address)->implicit_value (address),
            "Address to bind to")
        ("port,p", po::value<uint32_t>(&port)->implicit_value (port),
            "Port to bind to")
        ("name,n", po::value<std::string>(&name)->implicit_value (name),
            "Service name")
        ("accessible,a", po::value<uint64_t>(&accessibleBy)->implicit_value (accessibleBy),
            "Permission for accessing")
        ("type,t", po::value<std::string>(&type)->implicit_value (type),
            "Type of server");
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

    // Listen for connections
    boost::asio::io_service io_service;
    hlv::service::echo::server::ConnectionInformation info;
    hlv::service::echo::server::Server server (io_service,
                                  address,
                                  std::to_string(port), 
                                  info);
    launchService (io_service, server);
    
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
    // Create local lookup client
    std::unique_ptr<hlv::lookup::client::EvLookupClient>  localLookup = std::unique_ptr<hlv::lookup::client::EvLookupClient> (
                    new hlv::lookup::client::EvLookupClient(coordinator, coordinator_port));
    bool connect = localLookup->connect ();
    if (!connect) {
        std::cerr << "Failed to connect to lookup service" << std::endl;
        return 0;
    }
    auto client = discoverEdgeBox (*localLookup, 0); 
    if (!client) {
        std::cerr << "Failed to discover edge box " << std::endl;
        return 1;
    }

    // Connect to edge box
    bool conned = client->connect ();
    if (!conned) {
        std::cerr << "Failed to connect to edgebox " << std::endl;
        return 1;
    }

    std::list<std::string> changes;
    changes = {regAddress};
    bool succ = client->set_values (hlv::service::lookup::ECHO_LOCATION, changes);
    if (!succ) {
        std::cerr << "Failed to register" << std::endl;
        return 0;
    }

    std::cerr << "Running server " << std::endl;
    // This thread now provides I/O service
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
    io_service.run();
    return 1;
}
