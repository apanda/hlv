#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <query_client.h>
#include <update_client.h>
#include <sync_client.h>
#include <logging_common.h>
#include "consts.h"
#include "getifaddr.h"

namespace po = boost::program_options;
void authenticate (const std::string& lookup_server,
                   const uint32_t lookup_port,
                   const std::string& name,
                   const std::string& uname,
                   const std::string& passwd) {
    // Create clien
    hlv::lookup::client::EvLookupClient client (lookup_server, lookup_port);
    bool connect = client.connect ();
    if (!connect) {
        std::cerr << "Failed to connect to lookup service" << std::endl;
        return;
    }
    std::map<std::string, std::string> results;
    uint64_t token = 0;
    bool qsuccess = client.Query (0,
                                  name,
                                  token,
                                  results);
    if (!qsuccess) {
        std::cerr << "Failed to query lookup service " << std::endl;
        return;
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
            return;
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
            std::cout << "Token as int = " << token << std::endl;
        } else {
            std::cerr << "Failed to auth" << std::endl;
        }
    }
}

int main (int argc, char* argv[]) {
    init_logging();
    std::string server = "127.0.0.1",
                name   = "my_service",
                uname  = "nobody",
                password = "";
    uint32_t port = hlv::service::lookup::SERVER_PORT;
    po::options_description desc("Dropbox client");
    desc.add_options()
        ("help,h", "Display help") 
        ("lookup,l", po::value<std::string>(&server)->implicit_value ("127.0.0.1"),
            "Lookup server to contact")
        ("lport,lp", po::value<uint32_t>(&port)->implicit_value (port),
            "Lookup server port")
        ("name,n", po::value<std::string>(&name)->implicit_value (name),
            "Service name")
        ("uname,u", po::value<std::string>(&uname)->implicit_value (uname),
            "Username")
        ("password,p", po::value<std::string>(&password)->implicit_value (password),
            "Password");
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
    authenticate (server, port, name, uname, password);
    std::string address;
    bool asuccess = getFirstNonLoopbackAddress (address);
    if (asuccess) {
        std::cout << "Got IP address " << address << std::endl;;
    } else {
        std::cerr << "Failed to find IP " << std::endl;
        return 0;
    }
    hlv::ebox::update::EvLDiscoveryClient client (25, "127.0.0.1", hlv::service::lookup::EBOX_PORT);
    bool conned = client.connect ();
    if (!conned) {
        std::cerr << "Failed to connect " << std::endl;
        return 1;
    }
    std::list<std::string> changes = {address};
    bool succ = client.set_values ("bar", changes);
    if (succ) {
        std::cerr << "Running dropbox " << std::endl;
        succ = client.del_values ("bar", changes);
        if (!succ) {
            std::cerr << "Failed to delete" << std::endl;
        }
    } else {
        std::cerr << "Failed" << std::endl;
    }
    return 1;
}
