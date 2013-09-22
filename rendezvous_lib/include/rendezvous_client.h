// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <boost/asio.hpp>
#include <query_client.h>
#ifndef __EV_EBOX_RENDEZVOUS_LIB__
#define __EV_EBOX_RENDEZVOUS_LIB__
namespace ev {
namespace rendezvous {
bool rendezvous_with_other (hlv::lookup::client::EvLookupClient& client,
                            boost::asio::ip::tcp::socket& socket,
                            std::string& service_name,
                            uint64_t token,
                            const std::string& group,
                            uint64_t sessionKey);
}
}
#endif // __EV_EBOX_DISCOVERY_UPDATE_LIB__
