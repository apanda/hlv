#include <string>
#include <utility>
#include <boost/log/trivial.hpp>
#include "update_client.h"
#include "ebox.pb.h"
namespace hlv {
namespace ebox {
namespace update {
/// Construct an EV update Client.
/// host; string address of ebox host
/// port: uint32_t port of ebox host
EvLDiscoveryClient::EvLDiscoveryClient (const uint64_t token,
                                        const std::string& host,
                                        const uint32_t port) :
                 token_ (token),
                 host_ (host),
                 port_ (port),
                 connected_ (false),
                 update_ (new ev_ebox::LocalUpdate),
                 response_ (new ev_ebox::Response),
                 io_service_ (),
                 socket_ (io_service_) {
}

/// Connect to EV update service
bool EvLDiscoveryClient::connect () {
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({host_, std::to_string (port_)});
    boost::system::error_code ec;
    socket_.connect(endpoint, ec);
    if (ec) {
        BOOST_LOG_TRIVIAL(error) << "Error connecting to remote endpoint";
        connected_ = false;
    } else {
        connected_ = true;
    }
    return connected_;
}

/// Disconnect from EV update service
void EvLDiscoveryClient::disconnect () {
    socket_.close();
}

/// Set values for a given key
/// token: uint64_t token authorizing update
/// key: std::string:      key to update
/// values: TypeValueMap:  types and values to set
bool EvLDiscoveryClient::set_values (const std::string& key, 
                                     const ValueList& values) const {
    update_->Clear ();
    BOOST_LOG_TRIVIAL(info) << "Registering with key " << key;
    update_->set_type (ev_ebox::LocalUpdate::ADD);
    update_->set_token (token_);
    update_->set_key (key);
    for (auto v : values) {
        update_->add_values (v);
    }
    send_update (*update_);
    recv_response (*response_);
    return response_-> success ();
}

/// Delete types from a given key
/// token: uint64_t token authorizing update
/// key: std::string: key to update
/// types: TypeList:  types to delete
bool EvLDiscoveryClient::del_values (const std::string& key, 
                                     const ValueList& values) const {
    update_->Clear ();
    update_->set_type (ev_ebox::LocalUpdate::REMOVE);
    update_->set_token (token_);
    update_->set_key (key);
    for (auto v : values) {
        update_->add_values (v);
    }
    send_update (*update_);
    recv_response (*response_);
    return response_-> success ();
}

// Send update to the server
bool EvLDiscoveryClient::send_update (const ev_ebox::LocalUpdate& update) const {
    uint64_t size = update.ByteSize ();
    *((uint64_t*)buffer_.data()) = size;
    update.SerializeToArray (buffer_.data() + sizeof(uint64_t), size);
    BOOST_LOG_TRIVIAL (info) << "Sending update";
    boost::system::error_code ec;
    boost::asio::write (socket_,
      boost::asio::buffer(buffer_),
      boost::asio::transfer_exactly (size + sizeof(uint64_t)),
      ec);
    if (ec) {
        BOOST_LOG_TRIVIAL (info) << "Error sending update " << ec;
        return false;
    }
    BOOST_LOG_TRIVIAL (info) << "Succeeded in sending update";
    return true;
}

// Receive response from the server
bool EvLDiscoveryClient::recv_response (ev_ebox::Response& response) const {
    uint64_t size = 0;
    boost::system::error_code ec;
    boost::asio::read (socket_,
            boost::asio::buffer (&size, sizeof(size)),
            ec);

    if (ec) {
        BOOST_LOG_TRIVIAL (info) << "Error receiving size " << ec;
        return false;
    }

    boost::asio::read (socket_,
            boost::asio::buffer (buffer_),
            boost::asio::transfer_exactly (size),
            ec);

    if (ec) {
        BOOST_LOG_TRIVIAL (info) << "Error receiving message " << ec;
        return false;
    }

    response.ParseFromArray (buffer_.data(), size);
    return true;
}

EvLDiscoveryClient::~EvLDiscoveryClient() {
    delete update_;
    delete response_;
}
}
}
}
