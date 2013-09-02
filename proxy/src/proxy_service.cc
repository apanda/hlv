
// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <boost/log/trivial.hpp>
#include <cassert>
#include <memory>
#include "proxy_service.h"
#include "sync_client.h"
namespace hlv {
namespace service {
namespace proxy {
bool ProxyService::AuthenticateToken (const int64_t requestID,
                    const std::string& identity, 
                    const std::string& token, 
                    hlv_service::ServiceResponse& response) {
    BOOST_LOG_TRIVIAL(info) << "Received auth request for " << identity 
                            << " with token " << token;
    bool success = (groups_->find("") != groups_->end());
    std::string result;
    std::shared_ptr<Provider> provider;
    if (success) {
        success = (*groups_)[""]->find_nearest_provider (-1, // Special token for auth
                                    provider,
                                    false,
                                    true);
    }
    response.set_requestid (requestID);
    if (success) {
        hlv::service::client::SyncClient client (
          io_service_,
          provider->address,
          std::to_string(provider->port)
        );
        client.connect();
        std::tie (success, result) = 
            client.authenticate (identity,
                                 token.c_str(),
                                 token.size());
        client.stop();
    }
    response.set_success (success);
    response.set_response (result);
    return true;
}

bool ProxyService::ProcessRequest (const int64_t requestID,
                        const hlv_service::ServiceRequest& request, 
                        hlv_service::ServiceResponse& response) {
    assert (request.msgtype () == hlv_service::ServiceRequest_RequestType_REQUEST);
    BOOST_LOG_TRIVIAL(info) << "Received service request " << requestID
                            << " with token " << request.request().token()
                            << " with requesttype " << request.request().requesttype()
                            << " with arguments " << request.request().requestargument(); 
    auto token = request.request().token();
    std::string result;
    bool success = (groups_->find(token) != groups_->end());
    if (success) {
        std::tie (success, result) = 
            (*groups_)[token]->execute_request (io_service_,
                                    token,
                                    request.request().requesttype(),
                                    request.request().requestargument(),
                                    false, // Not hierarchical
                                    true); // Fallback
    } else {
        success = (groups_->find("") != groups_->end());
        if (success) {
            std::tie (success, result) = 
                (*groups_)[token]->execute_request (io_service_,
                                        token,
                                        request.request().requesttype(),
                                        request.request().requestargument(),
                                        false, // Not hierarchical
                                        true); // Fallback
        }
    }
    response.set_requestid (requestID);
    response.set_success (success);
    response.set_response (result);
    return true;
}
} // namespace server
} // namespace service
} // namespace hlv
