// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <iostream>
#include <string>
#include <thread>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <query_client.h>
#include <update_client.h>
#include <sync_client.h>
#include <logging_common.h>
#include <simple_server.h>
#include <simple_client.h>
#include <linenoise.h>
#include "consts.h"
#include "getifaddr.h"
/**
 * This is a generic client program (don't let dropbox fool you). Really what it does
 * is broadcast and receives messages. It is pretty boring but it exercises all sorts of
 * code paths
 **/

namespace po = boost::program_options;

/// Find authentication server, and then autenticate
/// client: Lookup client
/// name: service name
/// uname: Username
/// passwd: Password
uint64_t authenticate (hlv::lookup::client::EvLookupClient& client,
                       const std::string& uname,
                       const std::string& passwd) {
    // Lookup auth service
    uint64_t token = 0;
    std::map<std::string, std::string> results;
    bool qsuccess = client.Query (0,
                                  hlv::service::lookup::AUTH_SERVICE,
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

        // Break authentication result into server and port
        boost::split(split_results, authservice, boost::is_any_of(":"), boost::token_compress_off);
        if (split_results.size () != 2) {
            std::cerr << "Do not understand response" << std::endl;
            return token;
        }
        std::cout << "Authenticating with " << split_results[0] << "  " << split_results[1] << std::endl;

        // Create an authentication accessor. Note we don't really supply the auth service so can do whatever
        // it is one wants
        hlv::service::client::SyncClient client (
                split_results[0],
                split_results[1]);
        client.connect ();
        std::string str_token;
        bool success;
        std::tie(success, str_token) = client.authenticate (uname,
                                                            passwd.c_str (),
                                                            passwd.size ());

        // Successfully authenticated
        if (success) {
            token = std::stoull(str_token);
        } else {
            std::cerr << "Failed to auth" << std::endl;
        }
    }

    // By default just return 0
    return token;
}

int main (int argc, char* argv[]) {
    init_logging();

    /// Argument parsing
    std::string server = "127.0.0.1",
                name   = "my_service",
                uname  = "nobody",
                password = "";
    std::string query;
    uint32_t lport = hlv::service::lookup::SERVER_PORT;
    po::options_description desc("Echo client");
    desc.add_options()
        ("help,h", "Display help") 
        ("noauth,a", "Do not authenticate")
        ("lookup,l", po::value<std::string>(&server)->implicit_value ("127.0.0.1"),
            "Lookup server to contact")
        ("lport,lp", po::value<uint32_t>(&lport)->implicit_value (lport),
            "Lookup server port")
        ("name,n", po::value<std::string>(&name)->implicit_value (name),
            "Service name")
        ("uname,u", po::value<std::string>(&uname)->implicit_value (uname),
            "Username")
        ("password,p", po::value<std::string>(&password)->implicit_value (password),
            "Password");

    po::options_description hidden;
    hidden.add_options()
        ("echo", po::value<std::string>(&query), 
            "String to echo");
    po::positional_options_description positional;
    positional.add("echo", 1);

    po::options_description options;
    options.add(desc).add(hidden);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).positional(positional).run(), vm);
    po::notify(vm);

    if (vm.count("help") || !vm.count("echo")) {
        std::cerr << "Usage: echo_client [options] string" << std::endl;
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
    
    uint64_t token = 0;
    // Authenticate
    if (!vm.count("noauth")) {
        token = authenticate (lookupClient, uname, password);

        std::cerr << "Got token " << token << std::endl;
    }
    
    // Client
    hlv::simple::client::EvSimpleClient dclient (name,
                                        hlv::service::lookup::ECHO_LOCATION,  // No local part
                                        token,
                                        lookupClient,
                                        nullptr);
    std::string result;
    bool res = dclient.local_echo_request (hlv::service::lookup::ECHO_LOCATION,
                                    query, 
                                    result);
    if (!res) {
        std::cerr << "Failure contacting echo server" << std::endl;
    }
    std:: cout << result << std::endl;
    return 1;
}
