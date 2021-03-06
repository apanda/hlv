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
#include "update_connection.h"
#include "update_server.h"
#include "consts.h"

namespace {
/// Response to HGET. HGET is called to retrieve PERM bits
void redisHashResponse (redisAsyncContext* context, void* reply, void* data) {
    hlv::service::ebox::update::Connection* connect = 
                        (hlv::service::ebox::update::Connection*)data;
    redisReply* rreply = (redisReply*) reply;
    connect->hashReply (rreply);
}

/// Response to HSETNX which is called to set permission bits
void redisHashSetResponse (redisAsyncContext* context, void* reply, void* data) {
    hlv::service::ebox::update::Connection* connect = 
                        (hlv::service::ebox::update::Connection*)data;
    redisReply* rreply = (redisReply*) reply;
    connect->hashSetReply (rreply);
}

/// Callback for SADD used to add to the local discovery box
void redisSAddResponse (redisAsyncContext* context, void* reply, void* data) {
    hlv::service::ebox::update::Connection* connect = 
                        (hlv::service::ebox::update::Connection*)data;
    redisReply* rreply = (redisReply*) reply;
    connect->saddReply (rreply);
}

/// Callback for srem remove elements from local discovery box
void redisSRemResponse (redisAsyncContext* context, void* reply, void* data) {
    hlv::service::ebox::update::Connection* connect = 
                        (hlv::service::ebox::update::Connection*)data;
    redisReply* rreply = (redisReply*) reply;
    connect->sremReply (rreply);
}
}

namespace hlv {
namespace service{
namespace ebox {
namespace update {
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

void Connection::write_response (const ev_ebox::Response& response) {
    auto self(shared_from_this());
    uint64_t size = response.ByteSize ();
    *((uint64_t*)write_buffer_.data()) = size;
    response.SerializeToArray (write_buffer_.data() + sizeof(uint64_t), size);
    BOOST_LOG_TRIVIAL (info) << "Writing response";
    boost::asio::async_write (socket_,
        boost::asio::buffer(write_buffer_),
        boost::asio::transfer_exactly (size + sizeof(uint64_t)),
        [this, self] (boost::system::error_code ec,
                          std::size_t bytes_transfered) {
           if (ec) {
               BOOST_LOG_TRIVIAL (info) << "Error sending join message " << ec;
               manager_.stop(shared_from_this());
           }
           BOOST_LOG_TRIVIAL (info) << "Successfully responded";
           response_.Clear ();
           read_size ();
        }
    );
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
    BOOST_LOG_TRIVIAL(info) << "Being asked to read " << bufferSize_ << " bytes";
    boost::asio::async_read (socket_, 
            boost::asio::buffer(buffer_),
            boost::asio::transfer_exactly(length),
            [this, self] (boost::system::error_code ec,
                          std::size_t bytes_transfered) {
                BOOST_LOG_TRIVIAL(info) << "Read data";
                if (!ec) {
                    
                    if (!update_.ParseFromArray(buffer_.data(), bytes_transfered)) {
                        BOOST_LOG_TRIVIAL(error) << "Could not parse request";
                        manager_.stop(shared_from_this());
                        return;
                    }
                    process_request ();
                    //execute_updates (update_);
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

// Process all requests
void Connection::process_request () {
    if (update_.values_size() == 0) {
        BOOST_LOG_TRIVIAL (info) << "Failing due to lack of types";
        fail_request ();
        return;
    }
    // Step 1 for either add or remove get permissions
    // The flow is
    // get_permtoken -> hashReply (perm found) -> hashSetReply (set perm?) ->
    // {update_set -> saddReply} (add to set) or {remove_from_set ->  sremReply} (remove from set)
    get_permtoken ();
}

// Try to get permission tokens
void Connection::get_permtoken () {
    redisAsyncCommand (config_.redisContext, 
                      redisHashResponse,
                      this,
                      "HGET %s:%s %s",
                      config_.prefix.c_str (),
                      update_.key ().c_str (),
                      hlv::service::lookup::PERM_BIT_FIELD.c_str ());
                        
}

// Got a response from get permission tokens
void Connection::hashReply (redisReply* reply) {
    // Called back in here when PERM tokens are gotten
    BOOST_LOG_TRIVIAL (info) << "Got response to looking up PERM bits";
    if (reply->type == REDIS_REPLY_ERROR) {
        BOOST_LOG_TRIVIAL(error) << "Redis sent us an error, 'tis sad, fail";
        fail_request ();
    } else if (reply->type == REDIS_REPLY_NIL) {
        // If not found and adding just try adding a permission bit
        if (update_.type () == ev_ebox::LocalUpdate::ADD) {
            BOOST_LOG_TRIVIAL (info) << "This key doesn't exist, which is fine";
            BOOST_LOG_TRIVIAL (info) << "Setting token to current token " << update_.token ();
            redisAsyncCommand (config_.redisContext,
                               redisHashSetResponse,
                               this,
                               "HSETNX %s:%s %s %llu",
                               config_.prefix.c_str (),
                               update_.key ().c_str (),
                               hlv::service::lookup::PERM_BIT_FIELD.c_str (),
                               update_.token ());
        } else {
            BOOST_LOG_TRIVIAL (info) << "This key doesn't exist, can't really remove";
            fail_request ();
        }
    } else if (reply->type == REDIS_REPLY_STRING) {
        uint64_t token = std::stoull (std::string(reply->str));
        if (token != update_.token ()) {
            // This is an implementation detail, should be more general, simpler this way for now
            BOOST_LOG_TRIVIAL (info) << "Can only join a local lookup group with the same token, failing"
                                    << "token = " << token << " given " << update_.token ();
            fail_request ();
        } else {
            if (update_.type () == ev_ebox::LocalUpdate::ADD) {
                update_set ();
            } else if (update_.type () == ev_ebox::LocalUpdate::REMOVE) {
                remove_from_set (); 
            }
        }
    } else {
        BOOST_LOG_TRIVIAL (error) << "Redis's reply made no sense. The reply type was " << reply->type;
        fail_request ();
    }
}

// Got a response from trying to exclusively adding permission bits
void Connection::hashSetReply (redisReply* reply) {
    BOOST_LOG_TRIVIAL (info) << "Got response from setting PERM bits";
    if (reply->type == REDIS_REPLY_ERROR) {
        BOOST_LOG_TRIVIAL(error) << "Redis sent us an error, 'tis sad, fail";
        fail_request ();
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        if (reply->integer == 1) {
            update_set ();
        } else {
            // Someone beat us
            BOOST_LOG_TRIVIAL (info) << "SETNX reports we lost the race";
            // Get token and see what it is
            get_permtoken ();
        }
    } else {
        BOOST_LOG_TRIVIAL (error) << "Redis's reply made no sense. The reply type was " << reply->type;
        fail_request ();
    }
}

// Add elements to set of local hosts
void Connection::update_set () {
    std::stringstream keystream;
    keystream << config_.prefix.c_str() << ":" 
              << update_.key () 
              << "." << hlv::service::lookup::LOCAL_SET;
    std::string key = keystream.str ();
    BOOST_LOG_TRIVIAL (info) << "Adding to " << key; 
    const char** args = new const char*[2 + update_.values_size ()];
    size_t index = 0;
    args [index++] = "sadd";
    args [index++] = key.c_str ();
    for (auto v: update_.values ()) {
        args[index++] = v.c_str ();
    }

    redisAsyncCommandArgv (config_.redisContext,
                       redisSAddResponse,
                       this,
                       index,
                       args,
                       NULL);
    delete[] args;
}

void Connection::saddReply (redisReply* reply) {
    BOOST_LOG_TRIVIAL (info) << "Got response from SADD";
    if (reply->type == REDIS_REPLY_ERROR) {
        BOOST_LOG_TRIVIAL (error) << "Redis sent us an error";
        fail_request ();
    } else {
        response_.set_token (0);
        response_.set_success (true);
        update_.Clear ();
        write_response (response_);
    }
}

// Remove from set
void Connection::remove_from_set () {
    std::stringstream keystream;
    keystream << config_.prefix.c_str() << ":" 
              << update_.key () 
              << "." << hlv::service::lookup::LOCAL_SET;
    std::string key = keystream.str ();
    BOOST_LOG_TRIVIAL (info) << "Removing from " << key; 
    const char** args = new const char*[2 + update_.values_size ()];
    size_t index = 0;
    args [index++] = "srem";
    args [index++] = key.c_str ();
    for (auto v: update_.values ()) {
        args[index++] = v.c_str ();
    }

    redisAsyncCommandArgv (config_.redisContext,
                       redisSRemResponse,
                       this,
                       index,
                       args,
                       NULL);
    delete[] args;
}

void Connection::sremReply (redisReply* reply) {
    BOOST_LOG_TRIVIAL (info) << "Got response from SREM";
    if (reply->type == REDIS_REPLY_ERROR) {
        BOOST_LOG_TRIVIAL (error) << "Redis sent us an error";
        fail_request ();
    } else if (reply->type == REDIS_REPLY_INTEGER) {
        if (reply->integer > 0) {
            response_.set_token (0);
            response_.set_success (true);
            update_.Clear ();
            write_response (response_);
        } else {
            BOOST_LOG_TRIVIAL (info) << "No members found to delete";
            fail_request ();
        }
    } else {
        BOOST_LOG_TRIVIAL (info) << "Weird BOOST response";
        fail_request ();
    }
}

void Connection::fail_request () {
    response_.set_token (0);
    response_.set_success (false);
    update_.Clear ();
    write_response (response_);
}


} // namespace update
} // namespace ebox
} // namespace service
} // namespace hlv
