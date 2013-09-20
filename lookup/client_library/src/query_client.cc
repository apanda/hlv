#include <string>
#include <utility>
#include <boost/log/trivial.hpp>
#include "query_client.h"
#include "lookup.pb.h"
namespace hlv {
namespace lookup {
namespace client {
/// Construct an EV Lookup Client.
/// host; string address of lookup host
/// port: uint32_t port of lookup host
EvLookupClient::EvLookupClient (const std::string& host,
                                const uint32_t port) :
                 host_ (host),
                 port_ (port),
                 connected_ (false),
                 query_ (new ev_lookup::Query),
                 response_ (new ev_lookup::Response),
                 io_service_ (),
                 socket_ (io_service_) {
}

/// Connect to EV Lookup service
bool EvLookupClient::connect () {
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

/// Disconnect from EV lookup service
void EvLookupClient::disconnect () {
    socket_.close();
}

/// Query EV lookup service
/// token: Authentication token
/// query: Query string
/// resultToken: Token sent back by server, can be used to authenticate 
///              results
/// result: Map of results
bool EvLookupClient::Query (const uint64_t token,
                            const std::string& query,
                            uint64_t& resultToken,
                            LookupResult& result) const {
    if (!connected_) {
        return false;
    }
    query_->Clear ();
    response_->Clear ();
    query_->set_token (token);
    query_->set_querystring (query);
    query_->set_type (ev_lookup::Query::GLOBAL);
    bool success = send_query (*query_);
    if (!success) {
        return false;
    }
    success = recv_response (*response_);
    if (!success) {
        return false;
    }
    
    if (!response_->success ()) {
        return false;
    }

    resultToken = response_->token ();
    for (auto kv : response_->values ()) {
        // No emplace support :(
        result.insert (std::make_pair (kv.type(), kv.value()));
    }
    return true;
}

// Send a query to the server
bool EvLookupClient::send_query (const ev_lookup::Query& query) const {
    uint64_t size = query.ByteSize ();
    *((uint64_t*)buffer_.data()) = size;
    query.SerializeToArray (buffer_.data() + sizeof(uint64_t), size);
    BOOST_LOG_TRIVIAL (info) << "Sending query";
    boost::system::error_code ec;
    boost::asio::write (socket_,
      boost::asio::buffer(buffer_),
      boost::asio::transfer_exactly (size + sizeof(uint64_t)),
      ec);
    if (ec) {
        BOOST_LOG_TRIVIAL (info) << "Error sending query " << ec;
        return false;
    }
    BOOST_LOG_TRIVIAL (info) << "Succeeded in sending query";
    return true;
}

// Receive response from the server
bool EvLookupClient::recv_response (ev_lookup::Response& response) const {
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

EvLookupClient::~EvLookupClient() {
    delete query_;
    delete response_;
}
}
}
}
