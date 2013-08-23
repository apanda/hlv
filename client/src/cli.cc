// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <iostream>
#include <tuple>
#include <string>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/tokenizer.hpp>
#include "server.h"
#include "sync_client.h"
#include "cli.h"
namespace {
void quit (hlv::service::server::Server& server,
           hlv::service::client::SyncClient& client) {
    server.stop();;
    client.stop();
}
}
void repl (hlv::service::server::Server& server,
           hlv::service::client::SyncClient& client) {
    bool running = true;
    std::string command;
    std::string auth_token;
    std::string result;
    bool success;
    do {
        std::cout << "> ";
        std::getline (std::cin, command);
        std::cin.clear();
        std::vector<std::string> tokens;
        boost::tokenizer<boost::escaped_list_separator<char>> tok (
                command, 
                boost::escaped_list_separator<char>('\\', ' ', '\"'));
        tokens.clear();
        for (auto token: tok) {
            tokens.push_back (token);
        }
        if (tokens.size() == 0) {
            std::cout << "\n";
            continue;
        }
        if (boost::equals (tokens[0], "help")) {
            std::cout << "help" << std::endl
                      << "quit" << std::endl
                      << "request" << std::endl
                      << "authenticate" << std::endl;
        } else if (boost::equals (tokens[0], "quit")) {
            quit (server, client);
            running = false;
        } else if (boost::equals (tokens[0], "authenticate")) {
            if (tokens.size() != 3) {
                std::cout << "authenticate <username> <token>\n";
                continue;
            }
            std::tie(success, auth_token) = 
                client.authenticate(tokens[1], tokens[2].c_str(), tokens[2].size());
            if (!success) {
                std::cout << "Authentication failed\n";
                quit (server, client);
                running = false;
            } else {
                std::cout << "Authentication succeeded." << std::endl;
            }
        } else if (boost::equals (tokens[0], "request")) {
            if (tokens.size() < 3) {
                std::cout << "request [token] <request_id> <argument>\n";
                continue;
            }
            if (tokens.size() == 3) {
                std::tie(success,  result) = 
                    client.request (std::stoi(tokens[1]), tokens[2]);
            } else {
                std::tie(success, result) = 
                    client.request (tokens[1], std::stoi(tokens[2]), tokens[3]);
            }

            if (!success) {
                std::cout << "Success failed\n";
            } else {
                std::cout << "Request succeeded result = " << result << std::endl;
            }
        }
    } while (running);
}
