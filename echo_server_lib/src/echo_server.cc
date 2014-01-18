// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <signal.h>
#include <utility>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <boost/log/trivial.hpp>
#include "echo_connection.h"
#include "echo_server.h"

namespace hlv {
namespace service{
namespace echo {
namespace server {
Connection::Connection (boost::asio::ip::tcp::socket socket,
                        ConnectionManager& manager,
                        ConnectionInformation& config) :
    socket_ (std::move(socket)),
    bufferSize_ (0),
    manager_ (manager),
    config_ (config) {
}

void Connection::start () {
    BOOST_LOG_TRIVIAL(info) << "Starting connection";
    read();
}

void Connection::stop () {
    socket_.close();
}

/// Read and write data from the socket
void Connection::read () {
    auto self(shared_from_this());
    BOOST_LOG_TRIVIAL(info) << "Reading input";
    socket_.async_read_some (boost::asio::buffer(buffer_),
            [this, self] (boost::system::error_code ec,
                          std::size_t bytes_transfered) {
                BOOST_LOG_TRIVIAL(info) << "Read data";
                if (!ec) {
                    std::string data (buffer_.data (), bytes_transfered);
                    std::cout << data << std::endl;
                    boost::asio::async_write(socket_, 
                        boost::asio::buffer(buffer_, bytes_transfered),
                        [this, self] (boost::system::error_code ec, 
                                     std::size_t bytes_tranfered) {
                            if (!ec) {
                                read ();
                            } else {
                                BOOST_LOG_TRIVIAL(info) << "Error writing response";
                                manager_.stop(shared_from_this());
                            }
                        });
                } else if (ec != boost::asio::error::operation_aborted) {
                    // Stop here
                    BOOST_LOG_TRIVIAL(info) << "Connection ended read: " << bytes_transfered;
                    manager_.stop(shared_from_this());
                }
                else {
                    BOOST_LOG_TRIVIAL(error) << "Unknown ASIO error " << ec << " closing socket";
                    manager_.stop(shared_from_this());
                }
            });
}

} // namespace server
} // namespace echo
} // namespace service
} // namespace hlv
