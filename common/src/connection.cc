// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <signal.h>
#include <utility>
#include <iostream>
#include <boost/log/trivial.hpp>
#include "connection.h"
#include "connection_manager.h"

namespace hlv {
namespace service{
namespace server {
Connection::Connection (boost::asio::ip::tcp::socket socket,
                        ConnectionManager& manager,
                        std::shared_ptr<ServiceInterface> services) :
    socket_ (std::move(socket)),
    bufferSize_ (0),
    manager_ (manager),
    services_ (services) {
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
                    
                    if (!request_.ParseFromArray(buffer_.data(), bytes_transfered)) {
                        BOOST_LOG_TRIVIAL(error) << "Could not parse request";
                        manager_.stop(shared_from_this());
                        return;
                    }
                    BOOST_LOG_TRIVIAL(info) << "Received " << request_.msgtype();
                    dispatch_request (request_);
                     
                    request_.Clear ();
                    read_size();

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

hlv_service::ServiceResponse& 
Connection::dispatch_request (const hlv_service::ServiceRequest& request) {
    switch (request.msgtype()) {
        case hlv_service::ServiceRequest_RequestType_AUTHENTICATE:
            services_->AuthenticateToken (request.requestid(),
                                  request.authenticate().identity(),
                                  request.authenticate().token(),
                                  response_);
            break;
        case hlv_service::ServiceRequest_RequestType_REQUEST:
            services_->ProcessRequest (request.requestid(),
                                       request,
                                       response_);
            break;
    }
    write_response (response_);
    return response_;
}
void Connection::write_response (const hlv_service::ServiceResponse& response) {
    auto self(shared_from_this());
    uint64_t size = response.ByteSize ();
    *((uint64_t*)write_buffer_.data()) = size;
    response.SerializeToArray (write_buffer_.data() + sizeof(uint64_t), size);
    BOOST_LOG_TRIVIAL (info) << "Writing response";
    boost::asio::async_write (socket_,
        boost::asio::buffer(write_buffer_),
        boost::asio::transfer_exactly (size + sizeof(uint64_t)),
        [this, self] (boost::system::error_code ec,
                          std::size_t bytes_transfered) {
           if (ec) {
               BOOST_LOG_TRIVIAL (info) << "Error sending data " << ec;
               manager_.stop(shared_from_this());
           }
           BOOST_LOG_TRIVIAL (info) << "Succeeded in sending";
           response_.Clear();
        }
    );
}

} // namespace server
} // namespace service
} // namespace hlv
