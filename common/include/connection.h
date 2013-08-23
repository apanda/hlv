// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <boost/asio.hpp>
#include "service.pb.h"
#include "service_interface.h"
#include "common_manager.h"
#ifndef _HLV_SERVICE_CONNECTION_H_
#define _HLV_SERVICE_CONNECTION_H_
namespace hlv {
namespace service{
namespace server {

/// A connection represents a single client connected to the service.
/// Connections are themselves stateless (out of necessity), and are mainly
/// responsible for reading bytes off the wire and dispatching them
/// appropriately.
class Connection
    : public std::enable_shared_from_this<Connection>
{
  private:
    typedef std::shared_ptr<hlv::service::server::Connection> ConnectionPtr;
  public:
    Connection (const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    
    // Construct a Connection given a socket
    explicit Connection (boost::asio::ip::tcp::socket socket, 
            hlv::service::common::ConnectionManager<ConnectionPtr>& manager,
            std::shared_ptr<ServiceInterface> services);

    // Start listening for things
    void start ();

    // Stop listening
    void stop ();

  private:
    // Listen for buffer
    void read_size ();

    // Read buffer off the wire
    void read_buffer (uint64_t length);

    // Dispatch requests
    hlv_service::ServiceResponse& 
    dispatch_request (const hlv_service::ServiceRequest& request);

    void write_response (const hlv_service::ServiceResponse& response);

    // Socket for this connection
    boost::asio::ip::tcp::socket socket_;

    // A temporary variable that holds the amount to be read
    uint64_t bufferSize_;

    // Manage several connections
    hlv::service::common::ConnectionManager<ConnectionPtr>& manager_;

    // Service interface
    std::shared_ptr<ServiceInterface> services_;

    // Buffer, expect never to need more than 128k
    std::array<char, 131072> buffer_;
    std::array<char, 131072> write_buffer_;
    hlv_service::ServiceRequest request_;
    hlv_service::ServiceResponse response_;
};
} // namespace server
} // namespace service
} // namespace hlv
#endif
