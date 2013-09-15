#include <iostream>
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include "server.h"
#include "logging_common.h"
#include "service.pb.h"
#include "simple_service.h"
#include "proxy_client.h"

namespace po = boost::program_options;
int
main (int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    init_logging();
    po::options_description desc("HLV service options");
    std::string saddr = "0.0.0.0", 
                sport = "8080", 
                paddr, 
                pport = "8082";
    desc.add_options()
        ("help,h", "Display help")
        ("paddr,a", po::value<std::string>(&paddr)->implicit_value("0.0.0.0"), "Proxy address")
        ("pport,p", po::value<std::string>(&pport)->implicit_value("8082"), "Proxy port");
    po::options_description hidden;
    hidden.add_options()
        ("saddr,s", po::value<std::string>(&saddr), "Service address")
        ("sport,r", po::value<std::string>(&sport)->implicit_value("8080"), "Service port");
    po::positional_options_description p;
    p.add("saddr", 1);
    p.add("sport", 1);
    po::options_description options;
    options.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(options).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help") || (!vm.count("saddr"))) {
        std::cerr << "Usage: hlv_service [options] service_address [service_port]" << std::endl;
        std::cerr << desc;
        return 0;
    }
    boost::asio::io_service io_service;
    std::cout << "Listening on " << saddr << ":" << sport << std::endl;
    hlv::service::server::Server server (
            io_service,
            saddr, 
            sport, 
            std::make_shared<hlv::service::server::SimpleService>());
    server.start();
    if (vm.count("paddr")) {
        std::cout << "Connecting to proxy with address " << paddr << " " << pport << std::endl;
        hlv::service::proxy::SyncProxy proxy (io_service, paddr, pport);
        bool res = proxy.connect ();
        if (!res) {
            std::cerr << "Connection failed " << std::endl;
        } else {
            res = proxy.register_service ("", 
                                          saddr,
                                          std::stoi (sport),
                                          10,
                                          std::vector<int32_t>());
            if (!res) {
                std::cerr << "Error registering server";
            }
        }
        proxy.stop ();
    }
    boost::asio::signal_set signals (io_service);
    signals.add (SIGINT);
    signals.add (SIGTERM);
#if defined(SIGQUIT)
    signals.add (SIGQUIT);
#endif
    signals.async_wait ([&](boost::system::error_code, int) {
        std::cout << "Quitting" << std::endl;
        server.stop ();
        io_service.stop ();
    });
    // This thread now provides I/O service
    io_service.run();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;

}
