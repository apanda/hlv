// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <signal.h>
#include <utility>
#include <iostream>
#include <boost/log/trivial.hpp>
#include <string>
#include "service.pb.h"
#include "sync_client.h"
namespace hlv {
namespace service{
namespace client {

SyncClient::SyncClient (
        boost::asio::io_service& io_service,
        std::string rhost, 
        std::string rport) :
        io_service_ (io_service),
        socket_ (io_service_),
        signals_ (io_service_),
        rhost_ (rhost),
        rport_ (rport) {

    // Add the set of signals to listen for so that server can exit.
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif
    handle_signal();
}

std::tuple<bool, const std::string>
SyncClient::authenticate (const std::string& identity, 
                           const char* token,
                           int tokenLength) {
    BOOST_LOG_TRIVIAL(info) << "Authentication with ID " << identity;
    request_.Clear();
    response_.Clear();
    request_.set_requestid (1); // Since we have only one request inflight, don't need
                                // to distinguish between them.
    request_.set_msgtype (hlv_service::ServiceRequest_RequestType_AUTHENTICATE);
    auto authenticate = request_.mutable_authenticate();
    authenticate->set_identity(identity);
    authenticate->set_token(std::string(token, tokenLength));
    if (!send_request(request_)) {
        BOOST_LOG_TRIVIAL(error) << "Failed to send auth request"; 
        return std::make_tuple(false, "");
    }
    if (!receive_response()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to receive auth response"; 
        return std::make_tuple(false, "");
    }
    if (!response_.success()) {
        BOOST_LOG_TRIVIAL(error) << "Authentication server failed to authenticate"; 
        return std::make_tuple(false, "");
    }
    token_ = response_.response(); 
    BOOST_LOG_TRIVIAL(info) << "Successfully authenticated, token is " << token_;
    return std::make_tuple(true, token_);
}

std::tuple<bool, const std::string>
SyncClient::request (const std::string& token,
                     int32_t rtype,
                     const std::string& argument) {
    BOOST_LOG_TRIVIAL(info) << "Request of type " << rtype
                            << " argument " << argument;
    request_.Clear();
    response_.Clear();
    request_.set_requestid (1); // Since we have only one request inflight, don't need
                                // to distinguish between them.
    request_.set_msgtype (hlv_service::ServiceRequest_RequestType_REQUEST);
    auto request = request_.mutable_request ();
    request->set_token (token);
    request->set_requesttype (rtype);
    request->set_requestargument (argument);

    if (!send_request(request_)) {
        BOOST_LOG_TRIVIAL(error) << "Failed to send request"; 
        return std::make_tuple(false, "");
    }
    if (!receive_response()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to receive response"; 
        return std::make_tuple(false, "");
    }
    if (!response_.success()) {
        BOOST_LOG_TRIVIAL(error) << "Request failed results"; 
        return std::make_tuple(false, "");
    }
    std::string response = response_.response(); 
    BOOST_LOG_TRIVIAL(info) << "Response is " << response;
    return std::make_tuple(true, response);
}

std::tuple<bool, const std::string>
SyncClient::request (int32_t rtype,
                    const std::string& argument) {
    return request (token_, rtype, argument);
}

bool SyncClient::connect () {
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({rhost_, rport_});
    boost::system::error_code ec;
    socket_.connect(endpoint, ec);
    if (ec) {
        BOOST_LOG_TRIVIAL(error) << "Error connecting to remote endpoint";
        return false;
    }
    return true;
}

void SyncClient::handle_signal () {
    signals_.async_wait([this](boost::system::error_code, int) {
        BOOST_LOG_TRIVIAL(info) << "Caught signal, closing client";
        socket_.close();
    });
}

void SyncClient::stop () {
    socket_.close();
    signals_.cancel();
}

bool SyncClient::send_request (const hlv_service::ServiceRequest& request) {
    uint64_t size = request.ByteSize ();
    *((uint64_t*)write_buffer_.data()) = size;
    request.SerializeToArray (write_buffer_.data() + sizeof(uint64_t), size);
    BOOST_LOG_TRIVIAL (info) << "Writing request";
    boost::system::error_code ec;
    boost::asio::write (socket_,
      boost::asio::buffer(write_buffer_),
      boost::asio::transfer_exactly (size + sizeof(uint64_t)),
      ec);
    if (ec) {
        BOOST_LOG_TRIVIAL (info) << "Error sending data " << ec;
        stop();
        return false;
    }
    BOOST_LOG_TRIVIAL (info) << "Succeeded in sending";
    return true;
}

bool SyncClient::receive_response () {
    uint64_t size = 0;
    boost::system::error_code ec;
    boost::asio::read (socket_,
            boost::asio::buffer (&size, sizeof(size)),
            ec);
    if (ec) {
        BOOST_LOG_TRIVIAL (info) << "Error receiving size " << ec;
        stop();
        return false;
    }

    boost::asio::read (socket_,
            boost::asio::buffer (buffer_),
            boost::asio::transfer_exactly (size),
            ec);

    if (ec) {
        BOOST_LOG_TRIVIAL (info) << "Error receiving message " << ec;
        stop();
        return false;
    }

    response_.ParseFromArray (buffer_.data(), size);
    return true;
}
}
}
}
