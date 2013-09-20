// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <map>
#include <list>
#include <memory>
#include <boost/asio.hpp>
#ifndef __EV_EBOX_DISCOVERY_UPDATE_LIB__
#define __EV_EBOX_DISCOVERY_UPDATE_LIB__
namespace ev_ebox {
    class LocalUpdate;
    class Response;
}
namespace hlv {
namespace ebox {
namespace update {
/// Client to synchronously query the EV ebox service. This client is not
/// thread safe and is blocking.
class EvLDiscoveryClient {
  public:
    typedef std::list<std::string> ValueList;
    // Delete no argument constructor
    EvLDiscoveryClient () = delete;

    // Disallow copying
    EvLDiscoveryClient (const EvLDiscoveryClient&) = delete;
    
    /// Construct an EV Update Client.
    /// host; string address of update address
    /// port: uint32_t port for update server
    explicit EvLDiscoveryClient (const uint64_t token,
                                 const std::string& host,
                                 const uint32_t port);

    /// Connect to EV update service
    bool connect ();

    /// Disconnect from EV update service
    void disconnect ();

    /// Delete values
    /// token: uint64_t token authorizing update
    /// key: std::string: key to update
    /// types: TypeList:  types to delete
    bool del_values (const std::string& key, 
                     const ValueList& values) const;

    /// Set values for a given key
    /// token: uint64_t token authorizing update
    /// key: std::string:      key to update
    /// values: TypeValueMap:  types and values to set
    bool set_values (const std::string& key, 
                     const ValueList& values) const;

    virtual ~EvLDiscoveryClient();

  private:
    // Send update to the server
    bool send_update (const ev_ebox::LocalUpdate&) const;

    // Receive response from the server
    bool recv_response (ev_ebox::Response&) const; 

    uint64_t token_;

    // Host and port
    std::string host_;
    uint32_t port_;
    bool connected_;

    // Space to deserialize protobuf
    mutable ev_ebox::LocalUpdate* update_;
    mutable ev_ebox::Response* response_;
    // Buffer, expect never to need more than 128k
    mutable std::array<char, 131072> buffer_;

    // Communicating with the other end
    // We are never going to call run on this io_service_ so no
    // need to use a common one
    mutable boost::asio::io_service io_service_;
    // Socket
    mutable boost::asio::ip::tcp::socket socket_;
};
} // update
} // ebox
} // hlv
#endif // __EV_EBOX_DISCOVERY_UPDATE_LIB__
