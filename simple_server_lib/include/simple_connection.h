// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <boost/asio.hpp>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include "common_manager.h"
#ifndef _EV_SIMPLE_CONNECTION_H_
#define _EV_SIMPLE_CONNECTION_H_
/// The Connection class implements the logic used for a simple server
namespace hlv {
namespace service{
namespace simple {
namespace server {

/// Information used by each of the connection objects for initialization.
struct ConnectionInformation {
    ConnectionInformation() {
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
    typedef std::shared_ptr<hlv::service::simple::server::Connection> ConnectionPtr;
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

  private:
    // Listen for buffer
    void read_size ();

    // Read buffer off the wire
    void read_buffer (uint64_t length);

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
};
} // namespace server
} // namespace simple
} // namespace service
} // namespace hlv
#endif
