// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization

#include <memory>
#include <set>
#include <string>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include "proxy_connection.h"
#include "common_manager.h"
#include "common_server.h"
#ifndef _HLV_PROXY_SERVER_H_
#define _HLV_PROXY_SERVER_H_
namespace hlv {
namespace service {
namespace proxy {
typedef std::shared_ptr<hlv::service::proxy::Connection> ConnectionPtr;
typedef hlv::service::common::ConnectionManager<ConnectionPtr> ConnectionManager;

/// HLV proxy server
typedef hlv::service::common::Server <Connection, 
                                      ConnectionManager, 
                                      ConnectionInformation&>
                            Server;
} // namespace proxy
} // namespace service
} // namespace hlv
#endif

