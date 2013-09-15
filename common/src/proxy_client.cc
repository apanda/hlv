// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <signal.h>
#include <utility>
#include <iostream>
#include <boost/log/trivial.hpp>
#include <string>
#include "service.pb.h"
#include "proxy_client.h"
namespace hlv {
namespace service{
namespace proxy {

SyncProxy::SyncProxy (
        boost::asio::io_service& io_service,
        std::string phost, 
        std::string pport) :
        io_service_ (io_service),
        socket_ (io_service_),
        phost_ (phost),
        pport_ (pport) {

}

bool SyncProxy::connect () {
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({phost_, pport_});
    boost::system::error_code ec;
    socket_.connect(endpoint, ec);
    if (ec) {
        BOOST_LOG_TRIVIAL(error) << "Error connecting to remote endpoint";
        return false;
    }
    if (!receive_join ()) {
        BOOST_LOG_TRIVIAL(error) << "Error receiving join message";
        return false;
    }
    return true;
}

bool SyncProxy::register_service (std::string token,
                                  std::string address,
                                  int32_t port, 
                                  int32_t distance,
                                  const std::vector<int32_t>& services) {
    register_.Clear();
    register_.set_token (token);
    register_.set_address (address);
    register_.set_port (port);
    register_.set_distance (distance);
    for (auto service : services) {
        register_.add_requesttypes (service);
    }
    send_register (register_);
    return true;
}

const std::string& SyncProxy::get_server () const {
    return join_.serviceaddr();
}

const int32_t SyncProxy::get_port () const {
    return join_.serviceport();
}


void SyncProxy::stop () {
    socket_.close();
}

bool SyncProxy::send_register (const hlv_service::ProxyRegister& reg) {
    uint64_t size = reg.ByteSize ();
    *((uint64_t*)write_buffer_.data()) = size;
    reg.SerializeToArray (write_buffer_.data() + sizeof(uint64_t), size);
    BOOST_LOG_TRIVIAL (info) << "Writing register message";
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


bool SyncProxy::receive_join () {
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

    join_.ParseFromArray (buffer_.data(), size);
    return true;
}
}
}
}
