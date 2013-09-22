// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <map>
#include <list>
#include <memory>
#include <boost/asio.hpp>
#ifndef __EV_LOOKUP_UPDATE_LIB__
#define __EV_LOOKUP_UPDATE_LIB__
namespace ev_lookup {
    class Update;
    class UpdateResponse;
}
namespace hlv {
namespace lookup {
namespace update {
/// Client to synchronously update the EV lookup service. This client is not
/// thread safe and is blocking.
class EvUpdateClient {
  public:
    typedef std::map<std::string, std::string> TypeValueMap;
    typedef std::list<std::string> TypeList;
    // Delete no argument constructor
    EvUpdateClient () = delete;

    // Disallow copying
    EvUpdateClient (const EvUpdateClient&) = delete;
    
    /// Construct an EV Update Client.
    /// host; string address of update address
    /// port: uint32_t port for update server
    explicit EvUpdateClient (const std::string& host,
                             const uint32_t port);

    /// Connect to EV update service
    bool connect ();

    /// Disconnect from EV update service
    void disconnect ();

    /// Delete types from a given key
    /// token: uint64_t token authorizing update
    /// key: std::string: key to update
    /// types: TypeList:  types to delete
    bool del_types (const uint64_t token,
                    const std::string& key, 
                    const TypeList& types) const;

    /// Delete key
    /// token: uint64_t token authorizing update
    /// key: std::string:  key to update
    bool del_key (const uint64_t token,
                  const std::string& key) const;

    /// Set permissions for a given key
    /// token: uint64_t token authorizing update
    /// key: std::string:  key to update
    /// perm: uint64_t: permission to set
    bool set_permissions (const uint64_t token,
                          const std::string& key,
                          const uint64_t perm) const;

    /// Set values for a given key
    /// token: uint64_t token authorizing update
    /// key: std::string:      key to update
    /// values: TypeValueMap:  types and values to set
    bool set_values (const uint64_t token,
                     const std::string& key, 
                     const TypeValueMap& values) const;

    virtual ~EvUpdateClient();

  private:
    // Send update to the server
    bool send_update (const ev_lookup::Update&) const;

    // Receive response from the server
    bool recv_response (ev_lookup::UpdateResponse&) const; 

    // Host and port
    std::string host_;
    uint32_t port_;
    bool connected_;

    // Space to deserialize protobuf
    mutable ev_lookup::Update* update_;
    mutable ev_lookup::UpdateResponse* response_;
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
} // lookup
} // hlv
#endif // __EV_LOOKUP_UPDATE_LIB__

