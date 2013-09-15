// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization

#include <signal.h>
#include <utility>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#ifndef _HLV_COMMON_SERVER_H_
#define _HLV_COMMON_SERVER_H_
namespace hlv {
namespace service {
namespace common {

/// A server to build other servers. Servers all look the same (mostly),
/// so this made sense.
template<typename Connection,
         typename ConnectionManager,
         typename ConnectionParameter>
class Server {
  private:
    // I/O services for serving ASIO
    boost::asio::io_service& io_service_;
    
    // Listen to incoming connections
    boost::asio::ip::tcp::acceptor acceptor_;

    // Listening socket
    boost::asio::ip::tcp::socket socket_;
    
    // Collect connections
    ConnectionManager manager_;

    ConnectionParameter services_;
  public:
    // Delete some default constructors
    Server() = delete;
    Server(const Server&) = delete;
    Server& operator= (const Server&) = delete;
    
    // Construct a Server listening on a specific host and port
    Server (boost::asio::io_service& io_service,
            const std::string& host, 
            const std::string& port, 
            ConnectionParameter services):
    io_service_(io_service),
    acceptor_(io_service_),
    socket_(io_service_),
    services_ (services) {
        boost::asio::ip::tcp::resolver resolver(io_service_);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({host, port});
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
    }

    // Run server accept loop
    void start () {
        acceptor_.listen();
        do_accept();
    }

    // Stop server accept loop
    void stop () {
        acceptor_.close(); // Stop accepting connections
        manager_.stop_all();
    }

    virtual ~Server () {
        stop ();
    }

  private:

    // Set up callbacks for accepts
    void do_accept ()  {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec) {
                BOOST_LOG_TRIVIAL(info) << "Accept received";
                if (!acceptor_.is_open()) {
                    BOOST_LOG_TRIVIAL(debug) << "Stop accepting\n";
                    return; // Signal closed acceptor
                }
                if (!ec) {
                    BOOST_LOG_TRIVIAL(info) << "Making connection";
                    manager_.add_connection(std::make_shared<Connection> (
                            std::move(socket_), 
                            manager_,
                            services_));
                }

                do_accept();
            });
    }

};
} // namespace server
} // namespace service
} // namespace hlv
#endif

