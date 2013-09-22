// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization

#include <memory>
#include <set>
#include <string>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include "rendezvous_connection.h"
#include "common_manager.h"
#include "common_server.h"
#ifndef _EV_RENDEZVOUS_SERVER_H_
#define _EV_RENDEZVOUS_SERVER_H_
/// Alias some common files so that they are introduced in our namespace. This
/// essentially uses the "sever constructor kit" to construct a server.
namespace hlv {
namespace service {
namespace ebox {
namespace rendezvous {

/// EV ebox rendezvous service ebox pointer
typedef std::shared_ptr<hlv::service::ebox::rendezvous::Connection> ConnectionPtr;

/// EV ebox rendezvous service connection manager
typedef hlv::service::common::ConnectionManager<ConnectionPtr> ConnectionManager;

/// EV ebox rendezvous service server
typedef hlv::service::common::Server <Connection, 
                                      ConnectionManager, 
                                      ConnectionInformation&>
                            Server;
} // namespace rendezvous
} // namespace ebox
} // namespace service
} // namespace hlv
#endif
