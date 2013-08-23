// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization

#include <memory>
#include <set>
#include <string>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include "connection.h"
#include "connection_manager.h"
#include "service_interface.h"
#include "common_server.h"
#ifndef _HLV_SERVICE_SERVER_H_
#define _HLV_SERVICE_SERVER_H_
namespace hlv {
namespace service {
namespace server {

/// A server for HLV services. This is in the common library rather than
/// being in the server specific sources since both the client and the server
/// expose this interface.
typedef hlv::service::common::Server <Connection, 
                                      ConnectionManager, 
                            std::shared_ptr<ServiceInterface>>
                            Server;
} // namespace server
} // namespace service
} // namespace hlv
#endif

