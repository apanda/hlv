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
    void redisReflector (redisAsyncContext* context, void* reply, void* data) {
        auto connect = 
                      (hlv::service::lookup::update::Connection*)data;
        redisReply* rreply = (redisReply*) reply;
        connect->redisResponse (rreply);
    }
}

namespace hlv {
namespace service{
namespace lookup {
namespace update {
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

void Connection::write_response (const ev_lookup::UpdateResponse& response) {
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

void Connection::execute_updates (const ev_lookup::Update& updates) {
    switch (updates.operation ()) {
        case ev_lookup::Update::SET_VALUES:
            set_values (updates);
            break;
        case ev_lookup::Update::DELETE_TYPES:
            del_types (updates);
            break;
        case ev_lookup::Update::DELETE_KEY:
            del_key (updates);
            break;
        case ev_lookup::Update::SET_PERM:
            set_perm (updates);
            break;
    }
}

// Set one or more values
void Connection::set_values (const ev_lookup::Update& update) {
    assert (update.operation () == ev_lookup::Update::SET_VALUES);
    if (update.values_size() == 0) {
        BOOST_LOG_TRIVIAL (info) << "Failing SET_VALUES due to lack of types";
        response_.set_success (false); 
        write_response (response_);
        return;
    }
    std::stringstream keystream;
    keystream << config_.prefix.c_str() << ":" << update.key ();
    std::string key = keystream.str ();
    const char** args = new const char*[2 + 2 * update.values_size ()];
    size_t index = 0;
    args [index++] = "hmset";
    args [index++] = key.c_str ();
    for (auto kv : update.values ()) {
        args [index++] = kv.type ().c_str ();
        args [index++] = kv.value().c_str ();
    }
    redisAsyncCommandArgv (config_.redisContext,
                       redisReflector,
                       this,
                       index,
                       args,
                       NULL);
    delete[] args;
}

// Delete one or more types
void Connection::del_types (const ev_lookup::Update& update) {
    assert (update.operation () == ev_lookup::Update::DELETE_TYPES);
    if (update.values_size() == 0) {
        BOOST_LOG_TRIVIAL (info) << "Failing DELETE_TYPES due to lack of types";
        response_.set_success (false); 
        write_response (response_);
        return;
    }
    std::stringstream keystream;
    keystream << config_.prefix.c_str() << ":" << update.key ();
    std::string key = keystream.str ();
    const char** args = new const char*[2 +  update.values_size ()];
    size_t index = 0;
    args [index++] = "hdel";
    args [index++] = key.c_str ();
    for (auto kv : update.values ()) {
        args [index++] = kv.type ().c_str ();
    }
    redisAsyncCommandArgv (config_.redisContext,
                       redisReflector,
                       this,
                       index,
                       args,
                       NULL);
    delete[] args;
}

// Delete key
void Connection::del_key (const ev_lookup::Update& update) {
    assert (update.operation () == ev_lookup::Update::DELETE_KEY);
    redisAsyncCommand (config_.redisContext,
                       redisReflector,
                       this, 
                       "del \"%s:%s\"",
                       config_.prefix.c_str(),
                       update.key ().c_str());
}

// Set permissions for key
void Connection::set_perm (const ev_lookup::Update& update) {
    assert (update.operation () == ev_lookup::Update::SET_PERM);
    if (!update.has_permission ()) {
        BOOST_LOG_TRIVIAL (info) << "Failing SET_PERM operation since no permissions specified";
        response_.set_success (false);
        write_response (response_);
        return;
    }

    redisAsyncCommand (config_.redisContext,
                       redisReflector,
                       this, 
                       "hset \"%s:%s\" \"%s\" \"%lld\"",
                       config_.prefix.c_str(),
                       update.key ().c_str (),
                       hlv::service::lookup::PERM_BIT_FIELD.c_str(),
                       update.permission ());
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
                    execute_updates (update_);
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

void Connection::redisResponse (redisReply* reply) {
    BOOST_LOG_TRIVIAL (info) << "Got response";
    response_.set_success (reply->type != REDIS_REPLY_ERROR);
    update_.Clear ();
    write_response (response_);
}

} // namespace update
} // namespace lookup
} // namespace service
} // namespace hlv
