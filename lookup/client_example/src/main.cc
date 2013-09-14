#include <iostream>
#include <map>
#include <utility>
#include <boost/program_options.hpp>
#include <query_client.h>
namespace po = boost::program_options;

// Example EV lookup client
int main (int argc, char* argv[]) {
    // Argument parsing
    po::options_description desc("EV Lookup Client Example");
    std::string server = "127.0.0.1",
                query = "";
    uint64_t token = 0;
    uint32_t port = 8085;

    desc.add_options()
        ("h, help", "Display help") 
        ("s,server", po::value<std::string>(&server)->implicit_value("127.0.0.1"),
            "Lookup server to contact")
        ("p,port", po::value<uint32_t>(&port)->implicit_value(8085),
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

    std::map<std::string, std::string> results;
    uint64_t rtoken;
    bool qsuccess = client.Query (token,
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
    std::cout << "Done" << std::endl;
    return 1;
}
