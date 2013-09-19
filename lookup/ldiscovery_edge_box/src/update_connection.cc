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
    void redisHashResponse (redisAsyncContext* context, void* reply, void* data) {
        hlv::service::ebox::update::Connection* connect = 
                            (hlv::service::ebox::update::Connection*)data;
        redisReply* rreply = (redisReply*) reply;
        connect->hashReply (rreply);
    }

    void redisHashSetResponse (redisAsyncContext* context, void* reply, void* data) {
        hlv::service::ebox::update::Connection* connect = 
                            (hlv::service::ebox::update::Connection*)data;
        redisReply* rreply = (redisReply*) reply;
        connect->hashSetReply (rreply);
    }

    void redisSAddResponse (redisAsyncContext* context, void* reply, void* data) {
        hlv::service::ebox::update::Connection* connect = 
                            (hlv::service::ebox::update::Connection*)data;
        redisReply* rreply = (redisReply*) reply;
        connect->saddReply (rreply);
    }
}

namespace hlv {
namespace service{
namespace ebox {
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

void Connection::update_set () {
    std::stringstream keystream;
    keystream << config_.prefix.c_str() << ":" 
              << update_.key () 
              << "." << hlv::service::lookup::LOCAL_SET;
    std::string key = keystream.str ();
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

void Connection::set_values () {
    if (update_.values_size() == 0) {
        BOOST_LOG_TRIVIAL (info) << "Failing SET_VALUES due to lack of types";
        response_.set_success (false); 
        write_response (response_);
        return;
    }
    get_permtoken ();
}

void Connection::get_permtoken () {
    redisAsyncCommand (config_.redisContext, 
                      redisHashResponse,
                      this,
                      "HGET %s:%s %s",
                      config_.prefix.c_str (),
                      update_.key ().c_str (),
                      hlv::service::lookup::PERM_BIT_FIELD.c_str ());
                        
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

void Connection::fail_request () {
    response_.set_success (false);
    update_.Clear ();
    write_response (response_);
}

void Connection::hashReply (redisReply* reply) {
    BOOST_LOG_TRIVIAL (info) << "Got response to looking up PERM bits";
    if (reply->type == REDIS_REPLY_ERROR) {
        BOOST_LOG_TRIVIAL(error) << "Redis sent us an error, 'tis sad, fail";
        fail_request ();
    } else if (reply->type == REDIS_REPLY_NIL) {
        BOOST_LOG_TRIVIAL (info) << "This key doesn't exist, which is fine";
        BOOST_LOG_TRIVIAL (info) << "Setting token to current token";
        redisAsyncCommand (config_.redisContext,
                           redisHashSetResponse,
                           this,
                           "HSETNX %s:%s %s %d",
                           config_.prefix.c_str (),
                           update_.key ().c_str (),
                           update_.token ());
    } else if (reply->type == REDIS_REPLY_STRING) {
        uint64_t token = std::stoull (std::string(reply->str));
        if (token != update_.token ()) {
            // This is an implementation detail, should be more general, simpler this way for now
            BOOST_LOG_TRIVIAL (info) << "Can only join a local lookup group with the same token, failing";
            fail_request ();
        } else {
            update_set ();
        }
    } else {
        BOOST_LOG_TRIVIAL (error) << "Redis's reply made no sense. The reply type was " << reply->type;
        fail_request ();
    }
}

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
void Connection::saddReply (redisReply* reply) {
    BOOST_LOG_TRIVIAL (info) << "Got response from SADD";
    if (reply->type == REDIS_REPLY_ERROR) {
        BOOST_LOG_TRIVIAL (error) << "Redis sent us an error";
        fail_request ();
    } else {
        response_.set_success (false);
        update_.Clear ();
        write_response (response_);
    }
}

} // namespace update
} // namespace ebox
} // namespace service
} // namespace hlv
