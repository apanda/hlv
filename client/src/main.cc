#include <iostream>
#include <memory>
#include <thread>
#include <tuple>
#include <signal.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include "server.h"
#include "sync_client.h"
#include "logging_common.h"
#include "service.pb.h"
#include "client_service.h"
#include "proxy_client.h"
#include "cli.h"

namespace po = boost::program_options;

int
main (int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    init_logging();
    po::options_description desc("HLV client options");
    std::string saddr = "0.0.0.0", 
                sport = "8080", 
                raddr, 
                rport = "8082";
    desc.add_options()
        ("help,h", "Display help")
        ("saddr,a", po::value<std::string>(&saddr)->implicit_value("0.0.0.0"), "Local server address")
        ("sport,p", po::value<std::string>(&sport)->implicit_value("8080"), "Local server port");
    po::options_description hidden;
    hidden.add_options()
        ("raddr,s", po::value<std::string>(&raddr), "Proxy address")
        ("rport,r", po::value<std::string>(&rport)->implicit_value("8082"), "Proxy port");
    po::positional_options_description p;
    p.add("raddr", 1);
    p.add("rport", 1);
    po::options_description options;
    options.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help") || (!vm.count("raddr"))) {
        std::cerr << "Usage: hlv_client [options] remote_address [remote_port]" << std::endl;
        std::cerr << desc;
        return 0;
    }
    BOOST_LOG_TRIVIAL(info) << "Using local server address " << saddr << ":" << sport;
    BOOST_LOG_TRIVIAL(info) << "Using proxy " << raddr << ":" << rport;
    boost::asio::io_service io_service;
    hlv::service::server::Server server (
            io_service,
            saddr, 
            sport, 
            std::make_shared<hlv::service::client::ClientService>());
    hlv::service::proxy::SyncProxy proxy (
            io_service,
            raddr,
            rport);
    proxy.connect();
    BOOST_LOG_TRIVIAL(info) << "Using remote " << proxy.get_server() << ":" << proxy.get_port();
    hlv::service::client::SyncClient client (
            io_service,
            proxy.get_server(),
            std::to_string (proxy.get_port()));
    server.start();
    proxy.stop ();
    if (!client.connect()) {
        return 0;
    }
    // This thread now provides I/O service
    std::thread t0 ([&io_service] {
        io_service.run();
    });
    //std::string token;
    //std::tie(std::ignore, token) = 
    //    client.authenticate("panda", "blah", 4);
    //std::string result;
    //std::tie(std::ignore, result) =
    //    client.request (token, 2, ""); 
    repl (server, client);
    t0.join();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;

}
