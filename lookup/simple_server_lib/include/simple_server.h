// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization

#include <memory>
#include <set>
#include <string>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include "simple_connection.h"
#include "common_manager.h"
#include "common_server.h"
#ifndef _EV_SIMPLE_SERVER_H_
#define _EV_SIMPLE_SERVER_H_
/// Alias some common files so that they are introduced in our namespace. This
/// essentially uses the "sever constructor kit" to construct a server.
namespace hlv {
namespace service {
namespace simple {
namespace server {

/// Simple server connection
typedef std::shared_ptr<hlv::service::simple::server::Connection> ConnectionPtr;

/// Simple connection manager
typedef hlv::service::common::ConnectionManager<ConnectionPtr> ConnectionManager;

/// Simple server
typedef hlv::service::common::Server <Connection, 
                                      ConnectionManager, 
                                      ConnectionInformation&>
                            Server;
} // namespace server
} // namespace lookup
} // namespace service
} // namespace hlv
#endif
