// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <cassert>
#include <string>
#include <sstream>
#include <boost/log/trivial.hpp>
#include "auth_service.h"

namespace hlv {
namespace service {
namespace server {
// Receive an authentication token and generate a token
bool AuthService::AuthenticateToken (const int64_t requestID,
                    const std::string& identity, 
                    const std::string& token, 
                    hlv_service::ServiceResponse& response) {
    BOOST_LOG_TRIVIAL(info) << "Received auth request for " << identity 
                            << " with token " << token << " succeeding";
    BOOST_LOG_TRIVIAL(info) << "Sending request GET " << prefix_.c_str() << ":" << identity << token << std::endl;
    redisReply* reply = (redisReply*) redisCommand(syncContext_, "GET %s:%s%s",
                                prefix_.c_str(), 
                                identity.c_str(), 
                                token.c_str());
    uint64_t rtoken = 0;
    if (!reply) {
        BOOST_LOG_TRIVIAL(error) << "Error getting auth token";
        response.set_requestid (requestID);
        response.set_success (false);
        response.set_response (std::to_string(0));
        return true;
        
    } else if (reply->type == REDIS_REPLY_STRING) {
        BOOST_LOG_TRIVIAL(info) << "Actually got token ";
        rtoken = std::stoull(std::string(reply->str, reply->len));
    }  else if (reply->type == REDIS_REPLY_INTEGER) {
        BOOST_LOG_TRIVIAL(info) << "Actually got token ";
        rtoken = reply->integer;
    } else {
        BOOST_LOG_TRIVIAL(info) << "Could not find token ";
    }
    freeReplyObject (reply);
    BOOST_LOG_TRIVIAL(info) << "Sending token " << rtoken;
    
    response.set_requestid (requestID);
    response.set_success (true);
    response.set_response (std::to_string(rtoken));
    return true;
}


// Do nothing
bool AuthService::ProcessRequest (const int64_t requestID,
                        const hlv_service::ServiceRequest& request, 
                        hlv_service::ServiceResponse& response) {
    response.set_requestid (requestID);
    response.set_success (false);
    response.set_response ("");
    return true;
}
} // namespace server
} // namespace service
} // namespace hlv
