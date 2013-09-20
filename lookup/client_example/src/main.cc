#include <iostream>
#include <map>
#include <list>
#include <utility>
#include <boost/program_options.hpp>
#include <query_client.h>
#include "consts.h"
namespace po = boost::program_options;

// Example EV lookup client
int main (int argc, char* argv[]) {
    // Argument parsing
    po::options_description desc("EV Lookup Client Example");
    std::string server = "127.0.0.1",
                query = "";
    uint64_t token = 0;
    uint32_t port = hlv::service::lookup::SERVER_PORT;
    bool local = false;

    desc.add_options()
        ("h,help", "Display help") 
        ("l,local", po::value<bool>(&local)->zero_tokens(), "Local lookup")
        ("s,server", po::value<std::string>(&server)->implicit_value("127.0.0.1"),
            "Lookup server to contact")
        ("p,port", po::value<uint32_t>(&port)->implicit_value(port),
            "Lookup server port");

    po::options_description hidden;
    hidden.add_options()
        ("query", po::value<std::string>(&query), 
            "Query to run")
        ("token", po::value<uint64_t>(&token),
            "Token to use");
    po::positional_options_description positional;
    positional.add("query", 1);
    positional.add("token", 1);

    po::options_description options;
    options.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).positional(positional).run(), vm);
    po::notify(vm);

    // Need help or no query given?
    if (vm.count("help") || (!vm.count("query"))) {
        std::cerr << "Usage: lookup_client_mk1 [options] query token" << std::endl;
        std::cerr << desc << std::endl;
        return 0;
    }

    // Create client
    hlv::lookup::client::EvLookupClient client (server, port);
    bool connect = client.connect ();
    if (!connect) {
        std::cerr << "Failed to connect to lookup service" << std::endl;
        return 0;
    }

    uint64_t rtoken;
    bool qsuccess;
    if (local) {
        std::cerr << "Querying locally" << std::endl;
        std::list<std::string> results;
        qsuccess = client.LocalQuery (token,
                                 query,
                                 rtoken,
                                 results);
        if (!qsuccess) {
            std::cerr << "Failed to query or no results found" << std::endl;
            return 0;
        }
        std::cout << "Result token " << rtoken << std::endl;
        for (auto val : results) {
            std::cout << val << std::endl;
        }
    } else {
        std::map<std::string, std::string> results;
        qsuccess = client.Query (token,
                               query,
                               rtoken,
                               results);
        if (!qsuccess) {
            std::cerr << "Failed to query or no results found" << std::endl;
            return 0;
        }
        std::cout << "Result token " << rtoken << std::endl;
        for (auto kv : results) {
            std::cout << "  " << kv.first << ":   " << kv.second << std::endl;
        }
    }
    std::cout << "Done" << std::endl;
    return 1;
}
