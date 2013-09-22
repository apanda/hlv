// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <boost/asio.hpp>
#include <map>
#include <list>
#include <common_manager.h>
#include "ebox.pb.h"
#ifndef _EV_RENDEZVOUS_CONNECTION_H_
#define _EV_RENDEZVOUS_CONNECTION_H_
/// The Connection class implements the logic used by the HLV ebox service
namespace hlv {
namespace service{
namespace ebox {
namespace rendezvous {
class Connection;
typedef std::list<std::shared_ptr<Connection>> Connections;
typedef std::map<uint64_t, Connections> GroupMap;
typedef std::map<std::string, GroupMap> SessionMap;
/// Information used by each of the connection objects for initialization.
struct ConnectionInformation {
    SessionMap& sessionMap;
    ConnectionInformation(
            SessionMap& _sessionMap) :
            sessionMap(_sessionMap) {
    }

};

class Connection
    : public std::enable_shared_from_this<Connection>
{
  private:
    typedef std::shared_ptr<hlv::service::ebox::rendezvous::Connection> ConnectionPtr;
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
    
    // Join or create a rendezvous session
    void join_session ();

    // Leave a rendezvous session
    void leave_session ();

    // Listen for buffer
    void read_size ();

    // Read buffer off the wire
    void read_buffer (uint64_t length);

    // Connect both sides now
    void patch_through ();

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
    ev_ebox::RegisterSession session_;
};
} // namespace rendezvous
} // namespace ebox
} // namespace service
} // namespace hlv
#endif

