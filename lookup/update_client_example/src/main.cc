#include <iostream>
#include <map>
#include <utility>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <update_client.h>
#include "consts.h"
#include "linenoise.h"
namespace po = boost::program_options;

void completion (const char* buf, linenoise::linenoiseCompletions* lc) {
    switch (buf[0]) {
        case 'q':
            linenoise::linenoiseAddCompletion (lc, "quit");
            break;
            break;
        case 's':
            linenoise::linenoiseAddCompletion (lc, "set");
            linenoise::linenoiseAddCompletion (lc, "set_perm");
            break;
        case 'd':
            linenoise::linenoiseAddCompletion (lc, "del_key");
            linenoise::linenoiseAddCompletion (lc, "del_types");
            break;
    }
}

void set (const hlv::lookup::update::EvUpdateClient& client,
          const std::vector<std::string>& vec) {
    if (vec.size () < 4 || vec.size () % 2 != 0) {
        std::cerr << "set key type0 val0 [type1 val1 ...]" << std::endl;
        return;
    }
    const std::string& key = vec[1];
    hlv::lookup::update::EvUpdateClient::TypeValueMap vmap;
    for (size_t i = 2; i < vec.size(); i+=2) {
        vmap.insert(make_pair(vec[i], vec[i + 1]));
    }
    bool success = client.set_values (0, key, vmap); 
    if (success) {
        std::cout << "Successfully inserted " << std::endl;
    } else {
        std::cerr << "Failed to insert" << std::endl;
    }
}

void set_perm (const hlv::lookup::update::EvUpdateClient& client,
          const std::vector<std::string>& vec) {
    if (vec.size () != 3) {
        std::cerr << "set key perm" << std::endl;
        return;
    }
    const std::string& key = vec[1];
    uint64_t perm = std::stoull (vec[2]);
    bool success = client.set_permissions (0, key, perm);
    if (success) {
        std::cout << "Successfully set perm" << std::endl;
    } else {
        std::cerr << "Failed to set permissions" << std::endl;
    }
}

void del_key (const hlv::lookup::update::EvUpdateClient& client,
          const std::vector<std::string>& vec) {
    if (vec.size () != 2) {
        std::cerr << "del_key key" << std::endl;
        return;
    }
    const std::string& key = vec[1];
    bool success = client.del_key (0, key);
    if (success) {
        std::cout << "Successfully deleted key" << std::endl;
    } else {
        std::cerr << "Failed to delete key" << std::endl;
    }
}

void del_types (const hlv::lookup::update::EvUpdateClient& client,
                const std::vector<std::string>& vec) {
    if (vec.size () < 3) {
        std::cerr << "del_types key type0 [type1 ...]" << std::endl;
        return;
    }
    const std::string& key = vec[1];
    hlv::lookup::update::EvUpdateClient::TypeList tlist;
    for (size_t i = 2; i < vec.size(); i++) {
        tlist.push_back(vec[i]);
    }
    bool success = client.del_types (0, key, tlist); 
    if (success) {
        std::cout << "Successfully deleted type list " << std::endl;
    } else {
        std::cerr << "Failed to delete" << std::endl;
    }
}

// Example EV lookup client
int main (int argc, char* argv[]) {
    // Argument parsing
    po::options_description desc("EV Update Client Example");
    std::string server = "127.0.0.1",
                query = "";
    uint32_t port = hlv::service::lookup::UPDATE_PORT;

    desc.add_options()
        ("h, help", "Display help") 
        ("s,server", po::value<std::string>(&server)->implicit_value("127.0.0.1"),
            "Lookup server to contact")
        ("p,port", po::value<uint32_t>(&port)->implicit_value(port),
            "Lookup server port");

    po::options_description options;
    options.add(desc);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).run(), vm);
    po::notify(vm);

    // Need help or no query given?
    if (vm.count("help")) {
        std::cerr << "Usage: update_client_mk1 [options]" << std::endl;
        std::cerr << desc << std::endl;
        return 0;
    }

    // Create client
    hlv::lookup::update::EvUpdateClient client (server, port);
    bool connect = client.connect ();
    if (!connect) {
        std::cerr << "Failed to connect to lookup service" << std::endl;
        return 0;
    }
    const int32_t HISTORY_LEN = 1000;
    linenoise::linenoiseSetCompletionCallback (completion);
    linenoise::linenoiseHistorySetMaxLen (HISTORY_LEN);
    char* line;
    
    while (true) {
        line = linenoise::linenoise("lookup> ");
        if (line == NULL) {
            continue;
        }
        std::string lstring = std::string(line);
        boost::tokenizer<boost::escaped_list_separator<char>> tokenize(
                        lstring,
                        boost::escaped_list_separator<char> ('\\', ' ', '\"'));
        std::vector<std::string> split (tokenize.begin(), tokenize.end());
        if (split[0] == std::string("quit")) {
            break;
        } else if (split[0] == std::string("set")) {
            set (client, split);
        } else if (split[0] == std::string("set_perm")) {
            set_perm (client, split);
        } else if (split[0] == std::string("del_key")) {
            del_key (client, split);
        } else if (split[0] == std::string("del_types")) {
            del_types (client, split);
        } else {
            std::cerr << "Unrecognized command, valid commands are " << std::endl;
            std::cerr << "set" << std::endl;
            std::cerr << "set_perm" << std::endl;
            std::cerr << "del_key" << std::endl;
            std::cerr << "del_types" << std::endl;
        }
        free(line);
    }
    return 1;
}
