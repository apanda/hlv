// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <set>
#include <memory>
#ifndef _HLV_COMMON_MAN_H_
#define _HLV_COMMON_MAN_H_
namespace hlv {
namespace service{
namespace common {

/// When exiting it is useful to track what connections are open and shut them
/// down. The connection manager merely serves as the holder of this state.
template<typename ConnectionPtr>
class ConnectionManager {
private:
  std::set<ConnectionPtr> connections_;
public:
  ConnectionManager () {}

  void add_connection (ConnectionPtr connection) {
      connections_.insert(connection);
      connection->start();
  }

  void stop_all() {
      for (auto conn: connections_) {
          conn->stop();
      }
      connections_.clear();
  }
  
  void stop(ConnectionPtr c) {
      connections_.erase(c);
      c->stop();
  }
};
} // namespace server
} // namespace service
} // namespace hlv
#endif
