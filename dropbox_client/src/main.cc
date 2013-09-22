// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <iostream>
#include <string>
#include <thread>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <query_client.h>
#include <update_client.h>
#include <sync_client.h>
#include <logging_common.h>
#include <simple_server.h>
#include <simple_client.h>
#include "consts.h"
#include "getifaddr.h"

namespace po = boost::program_options;
uint64_t authenticate (hlv::lookup::client::EvLookupClient& client,
                       const std::string& name,
                       const std::string& uname,
                       const std::string& passwd) {
    uint64_t token = 0;
    std::map<std::string, std::string> results;
    bool qsuccess = client.Query (0,
                                  name,
                                  token,
                                  results);
    if (!qsuccess) {
        std::cerr << "Failed to query lookup service " << std::endl;
        return token;
    }
    auto authserverit = results.find (hlv::service::lookup::AUTH_LOCATION);
    std::string authservice;
    if (authserverit == results.end()) {
        std::cerr << "Failed to find auth service, assuming token = 0";
    } else {
        authservice = authserverit->second;
        std::vector<std::string> split_results;
        boost::split(split_results, authservice, boost::is_any_of(":"), boost::token_compress_off);
        if (split_results.size () != 2) {
            std::cerr << "Do not understand response" << std::endl;
            return token;
        }
        std::cout << "Authenticating with " << split_results[0] << "  " << split_results[1] << std::endl;
        hlv::service::client::SyncClient client (
                split_results[0],
                split_results[1]);
        client.connect ();
        std::string str_token;
        bool success;
        std::tie(success, str_token) = client.authenticate (uname,
                                                            passwd.c_str (),
                                                            passwd.size ());
        if (success) {
            const char* tokenArray = str_token.c_str ();
            token = *((uint64_t*)tokenArray);
        } else {
            std::cerr << "Failed to auth" << std::endl;
        }
    }
    return token;
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

void launchService (boost::asio::io_service& io_service,
                   hlv::service::simple::server::Server& server) {
    server.start ();
}

int main (int argc, char* argv[]) {
    init_logging();
    std::string server = "127.0.0.1",
                name   = "my_service",
                uname  = "nobody",
                password = "";
    uint32_t lport = hlv::service::lookup::SERVER_PORT,
              port = 9090;
    po::options_description desc("Dropbox client");
    desc.add_options()
        ("help,h", "Display help") 
        ("lookup,l", po::value<std::string>(&server)->implicit_value ("127.0.0.1"),
            "Lookup server to contact")
        ("lport,lp", po::value<uint32_t>(&lport)->implicit_value (lport),
            "Lookup server port")
        ("name,n", po::value<std::string>(&name)->implicit_value (name),
            "Service name")
        ("uname,u", po::value<std::string>(&uname)->implicit_value (uname),
            "Username")
        ("password", po::value<std::string>(&password)->implicit_value (password),
            "Password")
        ("port,p", po::value<uint32_t>(&port)->implicit_value (port),
            "Free port to use for sync");
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
    // Create lookup client
    hlv::lookup::client::EvLookupClient lookupClient (server, lport);
    bool connect = lookupClient.connect ();
    if (!connect) {
        std::cerr << "Failed to connect to lookup service" << std::endl;
        return 0;
    }
    
    // Authenticate
    uint64_t token;
    token = authenticate (lookupClient, name, uname, password);

    std::cerr << "Got token " << token << std::endl;
    
    // Discover address
    std::string address;
    bool asuccess = getFirstNonLoopbackAddress (address);
    if (asuccess) {
        std::cout << "Got IP address " << address << std::endl;;
    } else {
        std::cerr << "Failed to find IP " << std::endl;
        return 0;
    }
    std::stringstream addressStr;
    addressStr << address << ":" << port;
    std::string regAddress = addressStr.str ();

    // Discover edge box
    auto client = discoverEdgeBox (lookupClient, token); 
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

    // Listen for connections
    boost::asio::io_service io_service;
    hlv::service::simple::server::ConnectionInformation info;
    hlv::service::simple::server::Server syncServer (io_service,
                                  address,
                                  std::to_string(port), 
                                  info);
    launchService (io_service, syncServer);
    std::thread t0 ([&io_service] {
        // This thread now provides I/O service
        io_service.run();
    });

    // Register with edge box
    std::list<std::string> changes = {regAddress};
    std::stringstream domainkeystr;
    domainkeystr << uname << "." << name;
    std::string domainkey = domainkeystr.str ();
    bool succ = client->set_values (domainkey, changes);
    if (!succ) {
        std::cerr << "Failed to register" << std::endl;
        return 0;
    }

    // Run
    std::cerr << "Running dropbox " << std::endl;
    
    // Client
    hlv::simple::client::EvSimpleClient dclient (name,
                                        domainkey, 
                                        token,
                                        lookupClient);
    succ = dclient.send_everywhere ("Hello\n");
    if (!succ) {
        std::cerr << "Failed to send" << std::endl;
    }
    std::cin.ignore ();
    succ = client->del_values (domainkey, changes);
    if (!succ) {
        std::cerr << "Failed to delete" << std::endl;
    }
    io_service.stop ();
    t0.join ();
    return 1;
}
