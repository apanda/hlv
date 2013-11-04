#include <string>
#include <utility>
#include <boost/log/trivial.hpp>
#include "coordinator_client.h"
#include "lookup.pb.h"
namespace hlv {
namespace coordinator {
/// Construct an EV update Client.
/// host; string address of lookup host
/// port: uint32_t port of lookup host
EvUpdateClient::EvUpdateClient (const std::string& host,
                                const uint32_t port) :
                 host_ (host),
                 port_ (port),
                 connected_ (false),
                 update_ (new ev_lookup::Update),
                 response_ (new ev_lookup::UpdateResponse),
                 io_service_ (),
                 socket_ (io_service_) {
}

/// Connect to EV update service
bool EvUpdateClient::connect () {
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
void EvUpdateClient::disconnect () {
    socket_.close();
}


/// Set permissions for a given key
/// token: uint64_t token authorizing update
/// key: std::string:  key to update
/// perm: uint64_t: permission to set
bool EvUpdateClient::set_permissions (const uint64_t token,
                                      const std::string& key,
                                      const uint64_t perm) const {
    update_->Clear ();
    update_->set_operation (ev_lookup::Update::SET_PERM);
    update_->set_token (token);
    update_->set_key (key);
    update_->set_permission (perm);
    send_update (*update_);
    recv_response (*response_);
    return response_-> success ();
}

/// Set values for a given key
/// token: uint64_t token authorizing update
/// key: std::string:      key to update
/// values: TypeValueMap:  types and values to set
bool EvUpdateClient::set_values (const uint64_t token,
                                 const std::string& key, 
                                 const TypeValueMap& values) const {
    update_->Clear ();
    update_->set_operation (ev_lookup::Update::SET_VALUES);
    update_->set_token (token);
    update_->set_key (key);
    for (auto tv : values) {
        auto val = update_->add_values ();
        val->set_type (tv.first);
        val->set_value (tv.second);
    }
    send_update (*update_);
    recv_response (*response_);
    return response_-> success ();
}

/// Delete key
/// token: uint64_t token authorizing update
/// key: std::string:  key to update
bool EvUpdateClient::del_key (const uint64_t token,
                              const std::string& key) const {
    update_->Clear ();
    update_->set_operation (ev_lookup::Update::DELETE_KEY);
    update_->set_token (token);
    update_->set_key (key);
    send_update (*update_);
    recv_response (*response_);
    return response_-> success ();
}

/// Delete types from a given key
/// token: uint64_t token authorizing update
/// key: std::string: key to update
/// types: TypeList:  types to delete
bool EvUpdateClient::del_types (const uint64_t token,
                                const std::string& key, 
                                const TypeList& types) const {
    update_->Clear ();
    update_->set_operation (ev_lookup::Update::DELETE_TYPES);
    update_->set_token (token);
    update_->set_key (key);
    for (auto tv : types) {
        auto val = update_->add_values ();
        val->set_type (tv);
        val->set_value ("");
    }
    send_update (*update_);
    recv_response (*response_);
    return response_-> success ();
}

// Send update to the server
bool EvUpdateClient::send_update (const ev_lookup::Update& update) const {
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
bool EvUpdateClient::recv_response (ev_lookup::UpdateResponse& response) const {
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

EvUpdateClient::~EvUpdateClient() {
    delete update_;
    delete response_;
}
}
}

