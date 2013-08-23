// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include "server.h"
#include "sync_client.h"
#ifndef _HLV_CLIENT_REPL_H__
#define _HLV_CLIENT_REPL_H_
void repl (hlv::service::server::Server& server,
           hlv::service::client::SyncClient& client);
#endif
