#include <string>
#include <utility>
#include <boost/log/trivial.hpp>
#include <functional>
#include "query_client.h"
#include "lookup.pb.h"
namespace hlv {
namespace lookup {
namespace client {
namespace async {
/// Construct an EV Lookup Client.
/// host; string address of lookup host
/// port: uint32_t port of lookup host
EvLookupClient::EvLookupClient (boost::asio::io_service& io_service,
                                const std::string& host,
                                const uint32_t port) :
                 host_ (host),
                 port_ (port),
                 connected_ (false),
                 query_ (new ev_lookup::Query),
                 response_ (new ev_lookup::Response),
                 io_service_ (io_service),
                 socket_ (io_service_) {
}

/// Connect to EV Lookup service
void EvLookupClient::connect (std::function<void (bool, void*)> connected, void* context) {
    auto self (shared_from_this ());
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({host_, std::to_string (port_)});
    socket_.async_connect(endpoint,
        [this, self, connected, context] (boost::system::error_code ec) {
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "Error connecting to remote endpoint";
                connected_ = false;
            } else {
                connected_ = true;
            }
            connected (connected_, context);
    });
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
void EvLookupClient::Query (const uint64_t token,
                            const std::string& query,
                            LookupResult& result,
                            std::function<void(
                            bool,
                            uint64_t resultToken,
                            LookupResult& result)> f
                            ) {
    auto self (shared_from_this ());
    if (!connected_) {
        f (false, 0, result);
    }
    query_->Clear ();
    response_->Clear ();
    query_->set_token (token);
    query_->set_querystring (query);
    send_query (*query_,
        [self, this, f, &result] (bool success) {
            if (!success) {
                return f (false, 0, result);
            }
            recv_response (*response_,
                [&] (bool success) {
                    if (!success) {
                        return f (false, 0, result);
                    }
                    
                    if (!response_->success ()) {
                        return f (false, 0, result);
                    }

                    uint64_t resultToken = response_->token ();
                    for (auto kv : response_->values ()) {
                        // No emplace support :(
                        result.insert (std::make_pair (kv.type(), kv.value()));
                    }
                    f (true, resultToken, result);
              });
        });
}

// Send a query to the server
void EvLookupClient::send_query (const ev_lookup::Query& query, 
                                 std::function<void (bool)> f) {
    auto self (shared_from_this ());
    uint64_t size = query.ByteSize ();
    *((uint64_t*)buffer_.data()) = size;
    query.SerializeToArray (buffer_.data() + sizeof(uint64_t), size);
    BOOST_LOG_TRIVIAL (info) << "Sending query";
    boost::asio::async_write (socket_,
      boost::asio::buffer(buffer_),
      boost::asio::transfer_exactly (size + sizeof(uint64_t)),
      [this, self, f] (boost::system::error_code ec,
                          size_t bytes_transfered) {
            if (ec) {
                BOOST_LOG_TRIVIAL (info) << "Error sending query " << ec;
                f (true);
            }
            BOOST_LOG_TRIVIAL (info) << "Succeeded in sending query";
            f (false);
    });
}

// Receive response from the server
void EvLookupClient::recv_response (ev_lookup::Response& response,
                                   std::function<void (bool)> f) {
    auto self (shared_from_this ());
    uint64_t size = 0;
    boost::system::error_code ec;
    boost::asio::async_read (socket_,
            boost::asio::buffer (&size, sizeof(size)),
            [this, self, &size, &response, f] (boost::system::error_code ec,
                                 std::size_t bytes_transferred) {       
            if (ec) {
                BOOST_LOG_TRIVIAL (info) << "Error receiving size " << ec;
                f (false);
            }
            boost::asio::async_read (socket_,
                    boost::asio::buffer (buffer_),
                    boost::asio::transfer_exactly (size),
                    [&] (boost::system::error_code ec,
                        std::size_t bytes_transfered) {
                    if (ec) {
                        BOOST_LOG_TRIVIAL (info) << "Error receiving message " << ec;
                        f (false);
                    }

                    response.ParseFromArray (buffer_.data(), size);
                    f (true);
            });
    });
}

EvLookupClient::~EvLookupClient() {
    delete query_;
    delete response_;
}
}
}
}
}
