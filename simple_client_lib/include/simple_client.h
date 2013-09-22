// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <map>
#include <list>
#include <memory>
#include <boost/asio.hpp>
#include <query_client.h>
#ifndef __EV_SIMPLE_CLIENT_LIB__
#define __EV_SIMPLE_CLIENT_LIB__
namespace hlv {
namespace simple {
namespace client {
/// The client part of SimpleServer
class EvSimpleClient {
  public:
    typedef std::map<std::string, std::string> LookupResult;
    typedef std::list<std::string> LocalLookup;
    // Delete no argument constructor
    EvSimpleClient () = delete;

    // Disallow copying
    EvSimpleClient (const EvSimpleClient&) = delete;
    
    /// Construct an EV Lookup Client.
    /// host; string address of lookup host
    /// port: uint32_t port of lookup host
    explicit EvSimpleClient (const std::string name,
                             const std::string lname,
                             const uint64_t token,
                             hlv::lookup::client::EvLookupClient& 
                             client);

    virtual ~EvSimpleClient();

    // Send to only servers
    bool send_to_servers (const std::string& type, const std::string& str);

    // Send to only servers
    bool send_to_servers (const std::string& str);

    // Send to both peers and servers
    bool send_everywhere (const std::string& type, const std::string& str);

    // Send to both peers and servers
    bool send_everywhere (const std::string& str);

  private:
    // Send a query to the server
    bool send_query (const std::string& str,
                     boost::asio::ip::tcp::socket& socket) const;

    std::string servicename_;
    std::string lname_;
    uint64_t token_;

    hlv::lookup::client::EvLookupClient& client_;

    // Buffer, expect never to need more than 128k
    mutable std::array<char, 131072> buffer_;

    // Communicating with the other end
    // We are never going to call run on this io_service_ so no
    // need to use a common one
    mutable boost::asio::io_service io_service_;
};
}
}
}
#endif // __EV_QUERY_CLIENT_LIB__
