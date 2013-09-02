// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <memory>
#include <map>
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/asio.hpp>
#include <vector>
#include <utility>
#include <tuple>
#include "service.pb.h"
#ifndef __HLV_PROXY_AC_GROUP__
#define __HLV_PROXY_AC_GROUP__
namespace hlv {
namespace service {
namespace proxy {
struct Provider {
    std::string key;   // Key for looking up connection
    const std::string token; // A token to authenticate proxy
    const std::string address; // Address of service to use
    const int32_t port; // Port of service to use
    const int32_t distance; // Distance from proxy
    std::vector<int32_t> functions; // Functions provided
    Provider (const std::string _key,
              const std::string _token,
              const std::string _address,
              const int32_t _port, 
              const int32_t _distance,
              const size_t fCount):
              key (_key),
              token (_token),
              address (_address),
              port (_port),
              distance (_distance),
              functions (fCount) {
    }
};
class AnycastGroup :
    public std::enable_shared_from_this <AnycastGroup> {
  public:
    AnycastGroup (const AnycastGroup&) = delete;
    AnycastGroup& operator=(const AnycastGroup&) = delete;
    AnycastGroup () = delete;

    explicit AnycastGroup (
            const std::string& auth_token,
            std::shared_ptr<AnycastGroup> parent);

    bool add_member (const std::string key, 
                    const hlv_service::ProxyRegister&);

    bool find_nearest_provider (const int32_t function,
                         std::shared_ptr<Provider>& ptr,
                                const bool hierarchical,
                                const bool fallback);

    std::tuple<bool, const std::string> execute_request (
                                    boost::asio::io_service& io_service,
                                    const std::string& token,
                                    const int32_t function,
                                    const std::string& arguments,
                                    const bool hierarchical,
                                    const bool fallback);

   private:
     // Datastructure declaration
     typedef std::pair<int32_t, std::shared_ptr<Provider>> stored_pair;
     struct compare_first {
         bool operator() (const stored_pair& first, const stored_pair& second) const {
             return (first.first > second.first);
         }
     };
     typedef boost::heap::fibonacci_heap<stored_pair, 
                         boost::heap::compare<compare_first>,
                         boost::heap::stable<true>> fib_pq;

     bool find_nearest_provider_internal (const int32_t function,
                         std::shared_ptr<Provider>& ptr,
                             const bool hierarchical);

     std::tuple<bool, const std::string> execute_request_internal (
                                    boost::asio::io_service& io_service,
                                    const std::string& token,
                                    const int32_t function,
                                    const std::string& arguments,
                                    const bool hierarchical);

     std::tuple<bool, const std::string> execute_pq_requests (
                                    const fib_pq& pq,
                                    boost::asio::io_service& io_service,
                                    const std::string& token,
                                    const int32_t function,
                                    const std::string& arguments
                                    );

     
     // Authentication token this group is associated with
     const std::string& auth_token_;
     
     // Anycast group that is higher up the hierarchy.
     std::shared_ptr<AnycastGroup> parent_; 
     

     // The main map datastructure
     std::map<int32_t, fib_pq> membersByFunction_;

     fib_pq allMembers_;
};
typedef std::shared_ptr<std::map<std::string, std::shared_ptr<
                AnycastGroup>>>
        AnycastGroupType;
} // proxy
} // service
} // hlv
#endif
