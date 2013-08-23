// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <set>
#include <memory>
#include "connection.h"
#include "common_manager.h"
#ifndef _HLV_SERVICE_CON_MAN_H_
#define _HLV_SERVICE_CON_MAN_H_
namespace {
typedef std::shared_ptr<hlv::service::server::Connection> ConnectionPtr;
}
namespace hlv {
namespace service{
namespace server {
typedef hlv::service::common::ConnectionManager<ConnectionPtr> ConnectionManager;
} // namespace server
} // namespace service
} // namespace hlv
#endif
