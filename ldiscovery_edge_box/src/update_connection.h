// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <boost/asio.hpp>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include "ebox.pb.h"
#include "common_manager.h"
#ifndef _HLV_UPDATE_CONNECTION_H_
#define _HLV_UPDATE_CONNECTION_H_
/// The Connection class implements the logic used by the HLV ebox service
namespace hlv {
namespace service{
namespace ebox {
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
    typedef std::shared_ptr<hlv::service::ebox::update::Connection> ConnectionPtr;
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

    // Callback for Redis hget
    void hashReply (redisReply* reply);  
    
    // Callback for Redis hset
    void hashSetReply (redisReply* reply);  

    // Callback for Redis sadd
    void saddReply (redisReply* reply);

    // Callback for srem
    void sremReply (redisReply* reply);

  private:
    // Process request in update_
    void process_request ();

    // Actually execute an SADD
    void update_set ();

    // Execute SREM
    void remove_from_set ();

    // Fail request
    inline void fail_request ();

    // Get permission token from Redis
    inline void get_permtoken ();

    // Listen for buffer
    void read_size ();

    // Read buffer off the wire
    void read_buffer (uint64_t length);

    void write_response (const ev_ebox::Response&);

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
    ev_ebox::LocalUpdate update_;
    ev_ebox::Response response_;
};
} // namespace update
} // namespace ebox
} // namespace service
} // namespace hlv
#endif

