#include <iostream>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <query_client.h>
#include <update_client.h>
#include <logging_common.h>
#include "consts.h"

namespace po = boost::program_options;
int main (int argc, char* argv[]) {
    init_logging();
    hlv::ebox::update::EvLDiscoveryClient client (25, "127.0.0.1", hlv::service::lookup::UPDATE_PORT);
    bool conned = client.connect ();
    if (!conned) {
        std::cerr << "Failed to connect " << std::endl;
    }
    std::list<std::string> changes = {"food"};
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
}
