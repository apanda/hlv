// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <boost/log/trivial.hpp>
#include <cassert>
#include "null_service.h"

namespace hlv {
namespace service {
namespace server {
bool NullService::AuthenticateToken (const int64_t requestID,
                    const std::string& identity, 
                    const std::string& token, 
                    hlv_service::ServiceResponse& response) {
    BOOST_LOG_TRIVIAL(info) << "Received auth request for " << identity 
                            << " with token " << token << " succeeding";
    response.set_requestid (requestID);
    response.set_success (true);
    response.set_response ("");
    return true;
}

bool NullService::ProcessRequest (const int64_t requestID,
                        const hlv_service::ServiceRequest& request, 
                        hlv_service::ServiceResponse& response) {
    assert (request.msgtype () == hlv_service::ServiceRequest_RequestType_REQUEST);
    BOOST_LOG_TRIVIAL(info) << "Received service request " << requestID
                            << " with token " << request.request().token()
                            << " with requesttype " << request.request().requesttype()
                            << " with arguments " << request.request().requestargument(); 
    response.set_requestid (requestID);
    response.set_success (true);
    response.set_response ("");
    return true;
}
} // namespace server
} // namespace service
} // namespace hlv
