// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <tuple>
#include <vector>
#include <boost/asio.hpp>
#include "service.pb.h"
#include "service_interface.h"
#ifndef _HLV_PROXY_CLIENT_H_
#define _HLV_PROXY_CLIENT_H_
namespace hlv {
namespace service{
namespace proxy {
// A synchronous HLV proxy connector. 
class SyncProxy 
    : public std::enable_shared_from_this<SyncProxy>
{
  public:
    SyncProxy () = delete;
    SyncProxy (const SyncProxy&) = delete;
    SyncProxy& operator= (const SyncProxy&) = delete;
    
    // Construct one
    SyncProxy ( boost::asio::io_service& io_service,
            std::string phost, 
            std::string pport);

    // Connect to the remote host and port
    bool connect ();

    void stop ();

    bool register_service (std::string token,
                           std::string address,
                           int32_t port, 
                           int32_t distance,
                           const std::vector<int32_t>& services);

    const std::string& get_server () const;
    
    const int32_t get_port () const;

    virtual ~SyncProxy () {
        stop ();
    }

  private:
    // Handle signals
    void handle_signal();

    // Send request to HLV server
    bool send_register (const hlv_service::ProxyRegister& reg);

    // Receive initial join information
    bool receive_join ();
    
    // IO service provider
    boost::asio::io_service& io_service_;

    // Socket to server
    boost::asio::ip::tcp::socket socket_;

    // A temporary variable that holds the amount to be read
    uint64_t bufferSize_;

    // React to signals for shutdown etc
    boost::asio::signal_set signals_;

    // Endpoint information
    std::string phost_;
    std::string pport_;

    // Buffer, expect never to need more than 128k
    std::array<char, 131072> buffer_;
    std::array<char, 131072> write_buffer_;
    hlv_service::ProxyJoin join_;
    hlv_service::ProxyRegister register_;
};
} // namespace server
} // namespace service
} // namespace hlv
#endif
