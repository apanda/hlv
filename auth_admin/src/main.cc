// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <iostream>
#include <memory>
#include <string>
#include <boost/program_options.hpp>
#include <hiredis/hiredis.h>
#include "consts.h"

namespace po = boost::program_options;
int
main (int argc, char* argv[]) {
    // Option processing
    po::options_description desc("Auth service options");
    std::string redisAddress = "127.0.0.1",
                servicename = hlv::service::lookup::AUTH_SERVICE;
    int32_t redisPort = hlv::service::lookup::REDIS_PORT;

    desc.add_options()
        ("help,h", "Display help")
        ("raddress,r", po::value<std::string>(&redisAddress)->implicit_value("127.0.0.1"), "Redis server")
        ("rport", po::value<int32_t>(&redisPort)->implicit_value(6379), "Redis port")
        ("name,n", po::value<std::string>(&servicename)->implicit_value(servicename), "Service name"); 
    std::string user;
    std::string passwd;
    uint64_t token;
    po::options_description hidden;
    hidden.add_options()
        ("username", po::value<std::string>(&user), 
            "Username to add token for")
        ("password", po::value<std::string>(&passwd),
            "Password")
        ("token", po::value<uint64_t>(&token), 
            "Token");

    po::positional_options_description positional;
    positional.add("username", 1);
    positional.add("password", 1);
    positional.add("token", 1);
    po::options_description options;
    options.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).positional(positional).run(), vm);
    po::notify(vm);

    if (vm.count("help") || !vm.count("username") || !vm.count("password") || !vm.count("token")) {
        std::cerr << "Usage: redis_auth_admin <options> username password token" << std::endl; 
        std::cerr << desc;
        return 0;
    }
    //
    // Connect to redis
    redisContext *c  = redisConnect(redisAddress.c_str(), redisPort);
    if (c == NULL) {
        std::cerr << "Could not allocate redisContext" << std::endl;
        return 0;
    }

    if (c->err) {
        std::cerr << "Error connecting to redis " << c->errstr;
        return 0;
    }
    redisReply* reply = (redisReply*) redisCommand(c, "SET %s:%s%s %lld",
                                servicename.c_str(), 
                                user.c_str(), 
                                passwd.c_str(),
                                token);
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        std:: cerr << "Error setting value, boo" << std::endl;
        if (reply) {
            std::cerr << reply ->str << std::endl;
        }
        return 0;
    }
    freeReplyObject(reply);
    redisFree(c);
}
