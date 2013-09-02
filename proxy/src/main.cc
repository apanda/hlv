#include <iostream>
#include <memory>
#include <thread>
#include <tuple>
#include <signal.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include "server.h"
#include "logging_common.h"
#include "service.pb.h"
#include "proxy_server.h"
#include "anycast_group.h"
#include "proxy_service.h"

namespace po = boost::program_options;

int
main (int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    init_logging();
    po::options_description desc("HLV proxy options");
    std::string saddr = "0.0.0.0", 
                sport = "8080", 
                paddr, 
                pport = "8082";
    desc.add_options()
        ("help,h", "Display help")
        ("saddr,a", po::value<std::string>(&saddr)->implicit_value("0.0.0.0"), "Local server address")
        ("sport,p", po::value<std::string>(&sport)->implicit_value("8080"), "Local server port");
    po::options_description hidden;
    hidden.add_options()
        ("paddr", po::value<std::string>(&paddr), "Proxy address")
        ("pport", po::value<std::string>(&pport)->implicit_value("8082"), "Proxy port");
    po::positional_options_description p;
    p.add("paddr", 1);
    p.add("pport", 1);
    po::options_description options;
    options.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help") || (!vm.count("paddr"))) {
        std::cerr << "Usage: hlv_proxy [options] proxy_address [proxy_port]" << std::endl;
        std::cerr << desc;
        return 0;
    }
    BOOST_LOG_TRIVIAL(info) << "Using local server address " << saddr << ":" << sport;
    BOOST_LOG_TRIVIAL(info) << "Using proxy server " << paddr << ":" << pport;
    boost::asio::io_service io_service;
    auto anycastGroupDict = std::make_shared<
            std::map<std::string, std::shared_ptr<hlv::service::proxy::AnycastGroup>>>();
    hlv::service::server::Server server (
            io_service,
            saddr, 
            sport, 
            std::make_shared<hlv::service::proxy::ProxyService>(anycastGroupDict, io_service));
    hlv::service::proxy::ConnectionInformation join (anycastGroupDict,
                                                     "",
                                                     saddr, 
                                                     sport);
    hlv::service::proxy::Server proxy (
            io_service,
            paddr,
            pport,
            join);
    server.start();
    proxy.start();
    // This thread now provides I/O service
    io_service.run();
    //std::string token;
    //std::tie(std::ignore, token) = 
    //    client.authenticate("panda", "blah", 4);
    //std::string result;
    //std::tie(std::ignore, result) =
    //    client.request (token, 2, ""); 
    google::protobuf::ShutdownProtobufLibrary();
    return 0;

}
