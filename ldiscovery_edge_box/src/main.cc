// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <iostream>
#include <memory>
#include <thread>
#include <tuple>
#include <sstream>
#include <signal.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredisasio.h>
#include <getifaddr.h>
#include "consts.h"
#include "logging_common.h"
#include "update_server.h"

// Main file for EV ebox server
namespace po = boost::program_options;
namespace {
void connectCallback (const redisAsyncContext* c, int status) {
    if (status != REDIS_OK) {
        std::cerr << "Error " << c->errstr << std::endl;
        return;
    }
    BOOST_LOG_TRIVIAL (info) << "Connected with redis";
}

void disconnectCallback (const redisAsyncContext* c, int status) {
    BOOST_LOG_TRIVIAL (info) << "Disconnected ";
    if (status != REDIS_OK) {
        std::cerr << "Disconnected due to error " << c->errstr << std::endl;
    }
}
}

int
main (int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    init_logging();

    // Argument parsing
    po::options_description desc("Update service options");
    std::string address = "0.0.0.0",
                port    = std::to_string (hlv::service::lookup::EBOX_PORT),
                redisAddress = "127.0.0.1",
                prefix = hlv::service::lookup::REDIS_PREFIX;
    int32_t redisPort = hlv::service::lookup::REDIS_PORT;
    desc.add_options()
        ("help,h", "Display help")
        ("address,a", po::value<std::string>(&address)->implicit_value(address), "Bind to address")
        ("port,p", po::value<std::string>(&port)->implicit_value(port), "Bind to port")
        ("raddress,r", po::value<std::string>(&redisAddress)->implicit_value(redisAddress), "Redis server")
        ("rport", po::value<int32_t>(&redisPort)->implicit_value(redisPort), "Redis port")
        ("prefix", po::value<std::string>(&prefix)->implicit_value(prefix), 
                   "Prefix for redis DB");
    po::options_description options;
    options.add(desc);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).run(), vm);
    po::notify(vm);

    if (vm.count ("help")) {
        std::cerr << desc << std::endl;
        return 0;
    }

    if (!vm.count ("prefix")) {
        std::cerr << "NO PREFIX SPECIFIED FOR EDGE BOX, THIS IS A BAD IDEA" << std::endl;
        std::cerr << "Cowardly refusing to proceed" << std::endl;
        return 0;
    }

    // Register this box with lookup
    std::string registerAddress = address;
    if (registerAddress == "0.0.0.0") {
        bool asuccess = getFirstNonLoopbackAddress (registerAddress);
        if (!asuccess) {
            std::cerr << "Could not find address to register " << std::endl;
            return 0;
        }
    }

    redisContext *syncContext = redisConnect (redisAddress.c_str (), redisPort);
    if (syncContext == NULL) {
        std::cerr << "Failed to allocate a context for initial registeration" << std::endl;
        return 0;
    }
    if (syncContext != NULL && syncContext->err) {
        std::cerr << "Error connecting to redis " <<  syncContext->errstr;
        return 0;
    }
    
    redisReply* syncReply;
    syncReply = (redisReply*) redisCommand(syncContext, 
                                           "HSET %s:%s %s 0",   
                                           prefix.c_str (),
                                           hlv::service::lookup::LDEBOX_LOCATION.c_str (),
                                           hlv::service::lookup::PERM_BIT_FIELD.c_str ());
    if (!syncReply) {
        std::cerr << "Error updating permissions on lookup " << syncContext->errstr << std::endl;
        return 0;
    }

    freeReplyObject (syncReply);
    
    std::stringstream location;
    location << registerAddress << ":" << port;
    syncReply = (redisReply*) redisCommand (syncContext,
                                            "sadd %s:%s.%s %s",
                                             prefix.c_str (),
                                             hlv::service::lookup::LDEBOX_LOCATION.c_str (),
                                             hlv::service::lookup::LOCAL_SET.c_str (),
                                             location.str ().c_str ());
    if (!syncReply) {
        std::cerr << "Error adding edge box to set of edge boxes " << syncContext->errstr << std::endl;
        return 0;
    }
    freeReplyObject (syncReply);
    redisFree (syncContext);

    // Connect to Redis
    redisAsyncContext *context = redisAsyncConnect (redisAddress.c_str(), redisPort);
    if (!context) {
        std::cerr << "Failed to allocate redis context" << std::endl;
        return 0;
    }

    if (context->err) {
        std::cerr << "Failed to connect to redis instance " << syncContext->errstr << std::endl;
        return 0;
    }  

    redisAsyncSetConnectCallback (context, connectCallback);
    redisAsyncSetDisconnectCallback (context, disconnectCallback);

    // Create server
    boost::asio::io_service io_service;

    asio_redis::redisBoostClient client (io_service, context);
    // Server information
    hlv::service::ebox::update::ConnectionInformation information 
                                                    (redisAddress,
                                                     redisPort,
                                                     context,
                                                     prefix);
    BOOST_LOG_TRIVIAL (info) << "Using prefix " << prefix;
    // Create an update server
    hlv::service::ebox::update::Server update (
            io_service,
            address,
            port,
            information);
    update.start ();

    boost::asio::signal_set signals (io_service);
    signals.add (SIGINT);
    signals.add (SIGTERM);
#if defined(SIGQUIT)
    signals.add (SIGQUIT);
#endif
    signals.async_wait ([&](boost::system::error_code, int) {
        std::cout << "Quitting" << std::endl;
        update.stop ();
        client.stop ();
        io_service.stop ();
    });
    // This thread now provides I/O service
    io_service.run();
    redisAsyncDisconnect (context);

    syncContext = redisConnect (redisAddress.c_str (), redisPort);
    if (syncContext == NULL) {
        std::cerr << "Failed to allocate a context for deregistration" << std::endl;
        return 0;
    }
    if (syncContext != NULL && syncContext->err) {
        std::cerr << "Error connecting to redis " <<  syncContext->errstr;
        return 0;
    }
    syncReply = (redisReply*) redisCommand (syncContext,
                                            "srem %s:%s.%s %s",
                                             prefix.c_str (),
                                             hlv::service::lookup::LDEBOX_LOCATION.c_str (),
                                             hlv::service::lookup::LOCAL_SET.c_str (),
                                             location.str ().c_str ());
    if (!syncReply) {
        std::cerr << "Error removing edge box from set of edge boxes " << syncContext->errstr << std::endl;
        return 0;
    }
    freeReplyObject (syncReply);
    redisFree (syncContext);
    google::protobuf::ShutdownProtobufLibrary();
    return 1;

}
