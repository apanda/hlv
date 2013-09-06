// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <map>
#include <memory>
#include <boost/asio.hpp>
#include "service.pb.h"
#include "common_manager.h"
#ifndef _HLV_PROXY_CONNECTION_H_
#define _HLV_PROXY_CONNECTION_H_
namespace hlv {
namespace service{
namespace proxy {
struct ConnectionInformation {
    std::string token; // A token to authenticate proxy
    std::string address; // Address of service to use
    std::string client_port; // Port of service to use
    std::string server_port;
    ConnectionInformation(
                    std::string _token,
                    std::string _address,
                    std::string _cport,
                    std::string _sport) :
                    token (_token),
                    address (_address),
                    client_port (_cport),
                    server_port (_sport) {
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
    typedef std::shared_ptr<hlv::service::proxy::Connection> ConnectionPtr;
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

    void write_join (const hlv_service::ProxyJoin& proxy);

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
    hlv_service::ProxyJoin join_;
    hlv_service::ProxyRegister register_;
};
} // namespace proxy
} // namespace service
} // namespace hlv
#endif
