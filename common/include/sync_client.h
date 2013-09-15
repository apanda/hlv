// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <tuple>
#include <boost/asio.hpp>
#include "service.pb.h"
#include "service_interface.h"
#ifndef _HLV_CLIENT_CLIENT_H_
#define _HLV_CLIENT_CLIENT_H_
namespace hlv {
namespace service{
namespace client {
// A synchronous HLV client. 
class SyncClient 
    : public std::enable_shared_from_this<SyncClient>
{
  public:
    SyncClient () = delete;
    SyncClient (const SyncClient&) = delete;
    SyncClient& operator= (const SyncClient&) = delete;
    
    // Construct one
    SyncClient ( boost::asio::io_service& io_service,
            std::string rhost, 
            std::string rport);
    
    virtual ~SyncClient () {
        stop ();
    }

    // Connect to the remote host and port
    bool connect ();

    // Authenticate client. Returns a bool indicating success, and a string
    // indicating token 
    std::tuple<bool, const std::string> 
    authenticate (const std::string& identity, 
                  const char*        token,
                  int tokenLength);

    std::tuple<bool, const std::string>
    request (int32_t rtype,
            const std::string& argument);

    std::tuple<bool, const std::string>
    request (const std::string& token,
            int32_t rtype,
            const std::string& argument);

    void stop ();

  private:
    // Send request to HLV server
    bool send_request (const hlv_service::ServiceRequest& request);

    // Receive a response
    bool receive_response ();
    
    // IO service provider
    boost::asio::io_service& io_service_;

    // Socket to server
    boost::asio::ip::tcp::socket socket_;

    // A temporary variable that holds the amount to be read
    uint64_t bufferSize_;

    // Endpoint information
    std::string rhost_;
    std::string rport_;

    // Token after authentication
    std::string token_;

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
