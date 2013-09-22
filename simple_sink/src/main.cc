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
#include <rendezvous_client.h>
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
                       const std::string& name,
                       const std::string& uname,
                       const std::string& passwd) {
    // Lookup auth service
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
            const char* tokenArray = str_token.c_str ();
            token = *((uint64_t*)tokenArray);
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
                password = "",
                group = "default";
    uint32_t lport = hlv::service::lookup::SERVER_PORT;
    uint64_t skey = 1007;
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
        ("group,g", po::value<std::string>(&group)->implicit_value (group),
            "Group")
        ("session,s", po::value<uint64_t>(&skey) -> implicit_value (skey),
            "Session key");
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
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket (io_service);
    bool success = ev::rendezvous::rendezvous_with_other (lookupClient,
                                            socket,
                                            name,
                                            token,
                                            group,
                                            skey);
    if (!success) {
        std::cerr << "Rendezvous failed " << std::endl;
        return 0;
    }
    boost::system::error_code ec;
    std::array<char, 2000> buffer;
    size_t read = socket.read_some (boost::asio::buffer (buffer), 
            ec);
    if (ec) {
        std::cout << "Error read " << ec << std::endl;
    } else {
        std::string data (buffer.data (), read);
        std::cout << data << std::endl;
    }
    socket.close ();
    return 1;
}
