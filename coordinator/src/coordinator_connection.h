// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <boost/asio.hpp>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include "lookup.pb.h"
#include "common_manager.h"
#ifndef _EV_UPDATE_CONNECTION_H_
#define _EV_UPDATE_CONNECTION_H_
/// The Connection class implements the logic used by the EV lookup service
namespace hlv {
namespace service{
namespace coordinator {

/// Information used by each of the connection objects for initialization.
struct ConnectionInformation {
    // Redis server
    std::string redisServer; 
    // Redis port
    uint32_t redisPort;
    // hiredis context
    redisAsyncContext* redisContext;
    // Prefix: allows for multiple coordinators to share the same redis server.
    std::string prefix;
    ConnectionInformation(
            const std::string& _redisServer,
            const uint32_t  _redisPort,
            redisAsyncContext* _redisContext,
            std::string _prefix) :
            redisServer (_redisServer),
            redisPort (_redisPort),
            redisContext (_redisContext),
            prefix (_prefix) {
    }

};

/// The coordinator logic is implemented in the connection class
class Connection
    : public std::enable_shared_from_this<Connection>
{
  private:
    typedef std::shared_ptr<hlv::service::coordinator::Connection> ConnectionPtr;

  public:
    // Delete a bunch of constructors we don't want.
    Connection (const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection () = delete;
    
    // Construct a Connection given a socket
    explicit Connection (boost::asio::ip::tcp::socket socket, 
            hlv::service::common::ConnectionManager<ConnectionPtr>& manager,
            ConnectionInformation& config);

    // Start listening on the socket. The server creates a new Connection for each
    // client.
    void start ();

    // Stop listening on the socket. This is mostly important when exiting.
    void stop ();

    // Callback for Redis get
    void redisResponse (redisReply* reply);  

  private:
    // Process an update message
    void execute_updates (const ev_lookup::Update&);

    // Set permission
    void set_perm (const ev_lookup::Update&);

    // Delete key (all types are erased)
    void del_key (const ev_lookup::Update&);

    // Delete one or more types
    void del_types (const ev_lookup::Update&);

    // Set one or more values
    void set_values (const ev_lookup::Update&);

    // Listen for messages. Messages to the coordinator are always encoded as a
    // 64-bit length, followed by a Update (../proto/lookup.proto) message. 
    void read_size ();

    // Read update message from the network.
    void read_buffer (uint64_t length);

    // Write an UpdateResponse
    void write_response (const ev_lookup::UpdateResponse&);

    // Socket for this connection
    boost::asio::ip::tcp::socket socket_;

    // A temporary variable that holds the amount to be read
    uint64_t bufferSize_;

    // Manage several connections
    hlv::service::common::ConnectionManager<ConnectionPtr>& manager_;

    // Configuration
    const ConnectionInformation& config_;

    // Buffer, expect never to need more than 128k
    std::array<char, 131072> buffer_;
    std::array<char, 131072> write_buffer_;
    ev_lookup::Update update_;
    ev_lookup::UpdateResponse response_;
};
} // namespace coordinator
} // namespace service
} // namespace hlv
#endif

