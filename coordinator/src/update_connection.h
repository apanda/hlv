// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <boost/asio.hpp>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include "lookup.pb.h"
#include "common_manager.h"
#ifndef _HLV_UPDATE_CONNECTION_H_
#define _HLV_UPDATE_CONNECTION_H_
/// The Connection class implements the logic used by the HLV lookup service
namespace hlv {
namespace service{
namespace lookup {
namespace update {

/// Information used by each of the connection objects for initialization.
struct ConnectionInformation {
    std::string redisServer; // Redis server
    uint32_t redisPort; // Port
    redisAsyncContext* redisContext;
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

/// A connection represents a single client connected to the service.
/// Connections are themselves stateless (out of necessity), and are mainly
/// responsible for reading bytes off the wire and dispatching them
/// appropriately.
class Connection
    : public std::enable_shared_from_this<Connection>
{
  private:
    typedef std::shared_ptr<hlv::service::lookup::update::Connection> ConnectionPtr;
  public:
    Connection (const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection () = delete;
    
    // Construct a Connection given a socket
    explicit Connection (boost::asio::ip::tcp::socket socket, 
            hlv::service::common::ConnectionManager<ConnectionPtr>& manager,
            ConnectionInformation& config);

    // Start listening for things
    void start ();

    // Stop listening
    void stop ();

    // Callback for Redis get
    void redisResponse (redisReply* reply);  

  private:
    // Execute updates
    void execute_updates (const ev_lookup::Update&);

    // Set permission
    void set_perm (const ev_lookup::Update&);

    // Delete key (all types are erased)
    void del_key (const ev_lookup::Update&);

    // Delete one or more types
    void del_types (const ev_lookup::Update&);

    // Set one or more values
    void set_values (const ev_lookup::Update&);

    // Listen for buffer
    void read_size ();

    // Read buffer off the wire
    void read_buffer (uint64_t length);

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
} // namespace update
} // namespace lookup
} // namespace service
} // namespace hlv
#endif

