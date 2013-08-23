// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <string>
#include "service.pb.h"
#include "service_interface.h"
#ifndef _HLV_CLIENT_SERVICE_H_
#define _HLV_CLIENT_SERVICE_H_
namespace hlv {
namespace service {
namespace client {

// An example of what a service stub implemented by a client might look like. The
// idea in this case is to always disallow authentication (the client cannot
// authenticate itself). 
class ClientService : public hlv::service::server::ServiceInterface {
  public:
    ClientService () {}
    // Check an authentication token to see if a client can indeed own an
    // identity.  Generates a response which among other things contains the
    // authorization token that should be used in all subsequent requests.
    // Care should be taken to make sure this token is universal: i.e. it
    // authorizes all other service providers that can be used by this
    // client.
    virtual bool AuthenticateToken (const int64_t requestID,
                                const std::string& identity, 
                                const std::string& token, 
                                hlv_service::ServiceResponse& response);
    
    // Process a request. This call is responsible for making sure the
    // authorization token allows the client to receive the requested service. 
    virtual bool ProcessRequest (const int64_t requestID,
                                 const hlv_service::ServiceRequest& request, 
                                 hlv_service::ServiceResponse& response);
};
} // namespace server
} // namespace service
} // namespace hlv
#endif 
