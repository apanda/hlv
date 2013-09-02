// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include "anycast_group.h"
#include "sync_client.h"

namespace hlv {
namespace service {
namespace proxy {
AnycastGroup::AnycastGroup (
            const std::string& auth_token,
            std::shared_ptr<AnycastGroup> parent)
       : auth_token_ (auth_token),
         parent_ (parent) {
}
bool AnycastGroup::add_member (
        const std::string key,
        const hlv_service::ProxyRegister& reg) {
    // First collect all of this information
    int32_t distance = reg.distance ();
    std::string token = reg.token ();
    if (token != auth_token_) {
        return false;
    }
    std::string address = reg.address();
    int32_t port = reg.port();
    int32_t size = reg.requesttypes_size();
    auto element = std::make_shared<Provider> (key,
                                               token, 
                                               address, 
                                               port,
                                               distance, 
                                               size);
    allMembers_.emplace(distance, element);
    for (int i = 0; i < size; i++) {
        int32_t ftype = reg.requesttypes(i);
        element->functions.push_back (ftype);
        if (membersByFunction_.find (ftype) == membersByFunction_.end()) {
            membersByFunction_.emplace (ftype);
        }
        membersByFunction_[ftype].emplace (distance, element);
    }
    return true;
}
bool AnycastGroup::find_nearest_provider (const int32_t function,
                                  std::shared_ptr<Provider>& ptr,
                                         const bool hierarchical,
                                         const bool fallback) {
    if (find_nearest_provider_internal (function, ptr, hierarchical)) {
        return true;
    } else if (fallback && (!allMembers_.empty())) {
        ptr = allMembers_.top().second;
        return true;
    }
    return false;
}

bool AnycastGroup::find_nearest_provider_internal (const int32_t function,
                        std::shared_ptr<Provider>& ptr,
                        const bool hierarchical) {
    auto current (shared_from_this ()); 
    do {
        if (current->membersByFunction_.find (function) != 
                current->membersByFunction_.end()) {
            // Found something; check size
            if (!current->membersByFunction_[function].empty()) {
                ptr = current->membersByFunction_[function].top().second;
                return true;
            }
        }
        current = current->parent_;
    } while (current && hierarchical);
    return false;
}

std::tuple<bool, const std::string> AnycastGroup::execute_pq_requests (
                                       const fib_pq& pq,
                                       boost::asio::io_service& io_service,
                                       const std::string& token,
                                       const int32_t function,
                                       const std::string& arguments) {
    bool success = false;
    std::string result;
    for (auto val = pq.ordered_begin(); (val != pq.ordered_end()) && !success; val++) {
        auto provider = val->second;
        hlv::service::client::SyncClient client (
          io_service,
          provider->address,
          std::to_string(provider->port)
        );
        client.connect();
        std::tie(success, result) = 
            client.request (token, 
                            function, 
                            arguments);
    }
    return std::make_tuple(success, result);
}

std::tuple<bool, const std::string> AnycastGroup::execute_request (
                      boost::asio::io_service& io_service,
                                const std::string& token,
                                const int32_t function,
                                const std::string& arguments,
                                const bool hierarchical,
                                const bool fallback) {
    bool success = false;
    std::string result;
    std::tie(success, result) = execute_request_internal (io_service,
                                                          token,
                                                          function,
                                                          arguments,
                                                          hierarchical);
    if (!success && fallback) {
        std::tie(success, result) = 
            execute_pq_requests (allMembers_, io_service, token, function, arguments);
    }
    return std::make_tuple(success, result);
}

std::tuple<bool, const std::string> AnycastGroup::execute_request_internal (
                                        boost::asio::io_service& io_service,
                                                    const std::string& token,
                                                    const int32_t function,
                                                    const std::string& arguments,
                                                    const bool hierarchical) {
    auto current (shared_from_this ());
    std::string result;
    bool success = false;
    do {
        if (current->membersByFunction_.find (function) !=
                current->membersByFunction_.end ()) {
           auto ptr = current->membersByFunction_[function];
           execute_pq_requests (ptr, io_service, token, function, arguments);
        }
    } while ((!success) && current && hierarchical);
    return std::make_tuple(success, result); 
    
}
} // proxy
} // service
} // hlv
