// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <utility>
#include <iostream>
#include <string>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <cstdio>
#include <boost/log/trivial.hpp>
#include "rendezvous_connection.h"
#include "rendezvous_server.h"
#include "consts.h"

namespace hlv {
namespace service{
namespace ebox {
namespace rendezvous {
/// This is where the logic for the local discovery edge box is implemented.
/// This box is used to register iwth the local discovery service. The sequence of interactions
/// are as follows:
/// For adding:
///    1. Check for permission to add using HGET. If found go to step 3.
///    2. Try setting permission bits and reading set bits (to avoid races).
///    3. If permission bits match token, add to set of local services
/// For deleting
///    1. Check for permussion to add using HGET. If not found fail
///    2. If found check if permissions match.
///    3. If permissions match remove elements.

Connection::Connection (boost::asio::ip::tcp::socket socket,
                        ConnectionManager& manager,
                        ConnectionInformation& config) :
    socket_ (std::move(socket)),
    bufferSize_ (0),
    manager_ (manager),
    config_ (config) {
}

void Connection::start () {
    BOOST_LOG_TRIVIAL(info) << "Starting connection";
    read_size();
}

void Connection::stop () {
    socket_.close();
}

void Connection::read_size () {
    auto self(shared_from_this());
    boost::asio::async_read (socket_, 
            boost::asio::buffer(&bufferSize_, sizeof(bufferSize_)),
            [this, self] (boost::system::error_code ec, 
                                std::size_t bytes_transfered) {
                BOOST_LOG_TRIVIAL(info) << "Read connection";
                if (!ec) {
                    BOOST_LOG_TRIVIAL(info) << "Read " 
                                << bufferSize_
                                << " byte preheader " 
                                << bytes_transfered;
                    read_buffer(bufferSize_);
                } else if (ec != boost::asio::error::operation_aborted) {
                    // Stop here
                    BOOST_LOG_TRIVIAL(info) << "Connection ended";
                    manager_.stop(shared_from_this());
                }
                else {
                    BOOST_LOG_TRIVIAL(error) << "Unknown ASIO error " << ec << " closing socket";
                    manager_.stop(shared_from_this());
                }
            });
}

void Connection::read_buffer (uint64_t length) {
    auto self(shared_from_this());
    BOOST_LOG_TRIVIAL(info) << "Being asked to read " << length << " bytes";
    boost::asio::async_read (socket_, 
            boost::asio::buffer(buffer_),
            boost::asio::transfer_exactly(length),
            [this, self] (boost::system::error_code ec,
                          std::size_t bytes_transfered) {
                BOOST_LOG_TRIVIAL(info) << "Read data";
                if (!ec) {
                    if (!session_.ParseFromArray(buffer_.data(), bytes_transfered)) {
                        BOOST_LOG_TRIVIAL(error) << "Could not parse request";
                        manager_.stop(shared_from_this());
                        return;
                    }
                    join_session ();
                } else if (ec != boost::asio::error::operation_aborted) {
                    // Stop here
                    BOOST_LOG_TRIVIAL(info) << "Connection ended read: " << bytes_transfered;
                    manager_.stop(shared_from_this());
                }
                else {
                    BOOST_LOG_TRIVIAL(error) << "Unknown ASIO error " << ec << " closing socket";
                    manager_.stop(shared_from_this());
                }
            });
}

// Ideal this should just use splice(2) but I am lazy and this was simpler to do.
void Connection::patch_through () {
    auto self(shared_from_this ());
    socket_.async_read_some (
            boost::asio::buffer (buffer_),
            [this, self] (boost::system::error_code ec,
                          std::size_t bytes_transfered) {
                if (!ec) {
                    // Groups once created are never removed
                    auto group = config_.sessionMap.find (session_.group ());
                    // Same with sessions
                    auto session = group->second.find (session_.sessionkey ());
                    if (session->second.size () != 2) {
                        BOOST_LOG_TRIVIAL (info) << "Dropping information";
                    } else {
                        auto other = session->second.front ();
                        if (other == self) {
                            other = session->second.back ();
                        }
                        boost::asio::write (other->socket_,
                                boost::asio::buffer (buffer_), 
                                boost::asio::transfer_exactly (bytes_transfered),
                                ec);
                        if (ec) {
                            BOOST_LOG_TRIVIAL (error) << "Write failed " << ec;
                        }
                    }
                    patch_through ();
                    
                } else {
                    BOOST_LOG_TRIVIAL (info) << "Connection ended [Patch Through]";
                    manager_.stop (shared_from_this ());
                    leave_session ();
                }
            });
            
}

void Connection::join_session () {
    auto group = config_.sessionMap.find (session_.group ());
    if (group == config_.sessionMap.end ()) {
        BOOST_LOG_TRIVIAL (info) << "Creating new group " << session_.group ();
        bool success;
        std::tie(group, success) = 
            config_.sessionMap.insert (std::make_pair (session_.group (), GroupMap ())); 
    }
    auto session = group->second.find (session_.sessionkey ());
    if (session == group->second.end ()) {
        BOOST_LOG_TRIVIAL (info) << "Adding new session " << session_.group () << ":" << session_.sessionkey ();
        bool success;
        std::tie(session, success) = 
            group->second.insert (std::make_pair (session_.sessionkey (), Connections ()));
    }
    if (session->second.size () >= 2) {
        BOOST_LOG_TRIVIAL (error) << "Do not allow more than 2 connections ";
        return;
    }
    session->second.push_back (shared_from_this ());
    patch_through ();
}

void Connection::leave_session () {
    // Groups once created are never removed
    auto group = config_.sessionMap.find (session_.group ());
    // Same with sessions
    auto session = group->second.find (session_.sessionkey ());
    session->second.remove (shared_from_this ());
}

} // namespace rendezvous
} // namespace ebox
} // namespace service
} // namespace hlv
