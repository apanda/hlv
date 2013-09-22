// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <signal.h>
#include <utility>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <boost/log/trivial.hpp>
#include "simple_connection.h"
#include "simple_server.h"

namespace hlv {
namespace service{
namespace simple {
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
    read_size();
}

void Connection::stop () {
    socket_.close();
}

void Connection::read_size () {
    auto self(shared_from_this());
    boost::asio::async_read (socket_, 
            boost::asio::buffer(&bufferSize_, sizeof(bufferSize_)),
            [this, self] (boost::system::error_code ec, 
                                std::size_t bytes_transfered) {
                BOOST_LOG_TRIVIAL(info) << "Read connection";
                if (!ec) {
                    BOOST_LOG_TRIVIAL(info) << "Read " 
                                << bufferSize_
                                << " byte preheader " 
                                << bytes_transfered;
                    read_buffer(bufferSize_);
                } else if (ec != boost::asio::error::operation_aborted) {
                    // Stop here
                    BOOST_LOG_TRIVIAL(info) << "Connection ended";
                    manager_.stop(shared_from_this());
                }
                else {
                    BOOST_LOG_TRIVIAL(error) << "Unknown ASIO error " << ec << " closing socket";
                    manager_.stop(shared_from_this());
                }
            });
}

void Connection::read_buffer (uint64_t length) {
    auto self(shared_from_this());
    BOOST_LOG_TRIVIAL(info) << "Being asked to read " << bufferSize_ << " bytes";
    boost::asio::async_read (socket_, 
            boost::asio::buffer(buffer_),
            boost::asio::transfer_exactly(length),
            [this, self] (boost::system::error_code ec,
                          std::size_t bytes_transfered) {
                BOOST_LOG_TRIVIAL(info) << "Read data";
                if (!ec) {
                    std::string data (buffer_.data (), bytes_transfered);
                    std::cout << data << std::endl;
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
} // namespace simple
} // namespace service
} // namespace hlv
