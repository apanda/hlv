// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <map>
#include <memory>
#include <boost/asio.hpp>
#ifndef __EV_QUERY_CLIENT_LIB__
#define __EV_QUERY_CLIENT_LIB__
namespace ev_lookup {
    class Query;
    class Response;
}
namespace hlv {
namespace lookup {
namespace client {
/// Client to synchronously query the EV lookup service. This client is not
/// thread safe and is blocking.
class EvLookupClient {
  public:
    typedef std::map<std::string, std::string> LookupResult;
    // Delete no argument constructor
    EvLookupClient () = delete;

    // Disallow copying
    EvLookupClient (const EvLookupClient&) = delete;
    
    /// Construct an EV Lookup Client.
    /// host; string address of lookup host
    /// port: uint32_t port of lookup host
    explicit EvLookupClient (const std::string& host,
                             const uint32_t port);

    /// Connect to EV Lookup service
    bool connect ();

    /// Disconnect from EV lookup service
    void disconnect ();
    
    /// Query EV lookup service
    /// token: Authentication token
    /// query: Query string
    /// resultToken: Token sent back by server, can be used to authenticate 
    ///              results
    /// result: Map of results
    bool Query (const uint64_t token,
                const std::string& query,
                uint64_t& resultToken,
                LookupResult& result) const;

    virtual ~EvLookupClient();

  private:
    // Send a query to the server
    bool send_query (const ev_lookup::Query& query) const;

    // Receive response from the server
    bool recv_response (ev_lookup::Response& response) const; 

    // Host and port
    std::string host_;
    uint32_t port_;
    bool connected_;

    // Space to deserialize protobuf
    mutable ev_lookup::Query* query_;
    mutable ev_lookup::Response* response_;
    // Buffer, expect never to need more than 128k
    mutable std::array<char, 131072> buffer_;

    // Communicating with the other end
    // We are never going to call run on this io_service_ so no
    // need to use a common one
    mutable boost::asio::io_service io_service_;
    // Socket
    mutable boost::asio::ip::tcp::socket socket_;
};
}
}
}
#endif // __EV_QUERY_CLIENT_LIB__
