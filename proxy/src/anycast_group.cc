// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include "anycast_group.h"

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
    auto current(shared_from_this());
    do {
        if (membersByFunction_.find (function) != membersByFunction_.end()) {
            // Found something; check size
            if (!membersByFunction_[function].empty()) {
                ptr = membersByFunction_[function].top().second;
                return true;
            }
        }
        current = current->parent_;
    } while (current && hierarchical);
    return false;
}
} // proxy
} // service
} // hlv
