// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization

#include <memory>
#include <set>
#include <string>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include "update_connection.h"
#include "common_manager.h"
#include "common_server.h"
#ifndef _HLV_LOOKUP_UPDATE_SERVER_H_
#define _HLV_LOOKUP_UPDATE_SERVER_H_
/// Alias some common files so that they are introduced in our namespace. This
/// essentially uses the "sever constructor kit" to construct a server.
namespace hlv {
namespace service {
namespace ebox {
namespace update {

/// EV ebox update service ebox pointer
typedef std::shared_ptr<hlv::service::ebox::update::Connection> ConnectionPtr;

/// EV ebox update service connection manager
typedef hlv::service::common::ConnectionManager<ConnectionPtr> ConnectionManager;

/// EV ebox update service server
typedef hlv::service::common::Server <Connection, 
                                      ConnectionManager, 
                                      ConnectionInformation&>
                            Server;
} // namespace update
} // namespace ebox
} // namespace service
} // namespace hlv
#endif
