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
namespace async {
/// Client to synchronously query the EV lookup service. This client is not
/// thread safe and is blocking.
class EvLookupClient  
    : public std::enable_shared_from_this<EvLookupClient> {
  public:
    typedef std::map<std::string, std::string> LookupResult;
    // Delete no argument constructor
    EvLookupClient () = delete;

    // Disallow copying
    EvLookupClient (const EvLookupClient&) = delete;
    
    /// Construct an EV Lookup Client.
    /// host; string address of lookup host
    /// port: uint32_t port of lookup host
    explicit EvLookupClient (boost::asio::io_service& io,
                             const std::string& host,
                             const uint32_t port);

    /// Connect to EV Lookup service
    void connect (std::function<void (bool, void*)> connected, void* context);

    /// Disconnect from EV lookup service
    void disconnect ();
    
    /// Query EV lookup service
    /// token: Authentication token
    /// result: Map of results
    /// f: Callback function
    ///    bool: success
    ///    resultToken: Token for authentication
    ///    result: Results
    void Query (const uint64_t token,
                const std::string& query,
                LookupResult& results,
                std::function<void (
                bool,
                uint64_t resultToken,
                LookupResult& result)> f);

    virtual ~EvLookupClient();

  private:
    // Send a query to the server
    void send_query (const ev_lookup::Query& query, 
                 std::function<void (bool)> f);

    // Receive response from the server
    void recv_response (ev_lookup::Response& response,
                         std::function<void (bool)> f);

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
    boost::asio::io_service& io_service_;
    // Socket
    mutable boost::asio::ip::tcp::socket socket_;
};
}
}
}
}
#endif // __EV_QUERY_CLIENT_LIB__
