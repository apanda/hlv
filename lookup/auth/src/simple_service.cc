// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <cassert>
#include <string>
#include <boost/log/trivial.hpp>
#if defined(__APPLE__) && defined(__MACH__)
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#define SHA1 CC_SHA1
#else
#include <openssl/sha.h>
#endif
#include "simple_service.h"

namespace {
std::string sha256(const std::string str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    return std::string((char*)hash, SHA256_DIGEST_LENGTH);
}
}

namespace hlv {
namespace service {
namespace server {
bool SimpleService::AuthenticateToken (const int64_t requestID,
                    const std::string& identity, 
                    const std::string& token, 
                    hlv_service::ServiceResponse& response) {
    BOOST_LOG_TRIVIAL(info) << "Received auth request for " << identity 
                            << " with token " << token << " succeeding";
    response.set_requestid (requestID);
    response.set_success (true);
    response.set_response (::sha256 (identity));
    return true;
}

bool SimpleService::ProcessRequest (const int64_t requestID,
                        const hlv_service::ServiceRequest& request, 
                        hlv_service::ServiceResponse& response) {
    assert (request.msgtype () == hlv_service::ServiceRequest_RequestType_REQUEST);
    BOOST_LOG_TRIVIAL(info) << "Received service request " << requestID
                            << " with requesttype " << request.request().requesttype()
                            << " with arguments " << request.request().requestargument(); 
    response.set_requestid (requestID);
    response.set_success (false);
    response.set_response ("");
    return true;
}
} // namespace server
} // namespace service
} // namespace hlv
