// Copyright 20xx The Regents of the University of California
// This is a test service for SDN-v2 High Level Virtualization
#include <signal.h>
#include <utility>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <boost/log/trivial.hpp>
#include "lookup_connection.h"
#include "lookup_server.h"
#include "consts.h"

namespace {
    void getCallback (redisAsyncContext* context, void* reply, void* data) {
        hlv::service::lookup::server::Connection* connect = 
                            (hlv::service::lookup::server::Connection*)data;
        redisReply* rreply = (redisReply*) reply;
        connect->getSucceeded (rreply);
    }

    void localPermGetCallback (redisAsyncContext* context, void* reply, void* data) {
        hlv::service::lookup::server::Connection* connect = 
                            (hlv::service::lookup::server::Connection*)data;
        redisReply* rreply = (redisReply*) reply;
        connect->getPermFieldSucceeded (rreply);
    }

    void localSmemberCallback (redisAsyncContext* context, void* reply, void* data) {
        hlv::service::lookup::server::Connection* connect = 
                            (hlv::service::lookup::server::Connection*)data;
        redisReply* rreply = (redisReply*) reply;
        connect->smemberSucceeded (rreply);
    }


}

namespace hlv {
namespace service{
namespace lookup {
namespace server {
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

void Connection::write_response (const ev_lookup::Response& response) {
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

void Connection::global_lookup () {
    BOOST_LOG_TRIVIAL (info) << "Querying globally " 
                             << config_.prefix 
                             << ":"
                             << query_.querystring ();
    redisAsyncCommand (config_.redisContext, 
                        getCallback, 
                        this, 
                        "HGETALL %s:%s", 
                        config_.prefix.c_str(),
                        query_.querystring ().c_str());  
}

void Connection::local_lookup () {
    BOOST_LOG_TRIVIAL (info) << "Querying locally " 
                             << config_.localPrefix 
                             << ":"
                             << query_.querystring ()
                             << " " 
                             << hlv::service::lookup::PERM_BIT_FIELD;
    redisAsyncCommand (config_.redisContext, 
                        localPermGetCallback, 
                        this, 
                        "HGET %s:%s %s", 
                        config_.localPrefix.c_str(),
                        query_.querystring ().c_str(),
                        hlv::service::lookup::PERM_BIT_FIELD.c_str ());  
}

void Connection::getPermFieldSucceeded (redisReply* reply) {
    BOOST_LOG_TRIVIAL (info) << "Got response to request for permissions";
    if (reply->type == REDIS_REPLY_ERROR) {
        BOOST_LOG_TRIVIAL (error) << "Redis sent us an error, 'tis sad, fail";
        fail_request ();
    } else if (reply->type == REDIS_REPLY_NIL) {
        BOOST_LOG_TRIVIAL (info) << "Getting permissions field failed, failing";
        fail_request ();
    } else if (reply->type == REDIS_REPLY_STRING) {
        uint64_t token = std::stoull(std::string(reply->str));
        if (token == query_.token ()) {
            BOOST_LOG_TRIVIAL (info) << "Successfully authenticated local token, now "
                                    << "executing actual query ";
            lookup_local_set ();
        } else {
            BOOST_LOG_TRIVIAL (info) << "Looks like you are not allowed in here";
            fail_request ();
        }
    } else {
        BOOST_LOG_TRIVIAL (info) << "Unrecognized redis return type";
        fail_request ();
    }
}

void Connection::lookup_local_set () {
    BOOST_LOG_TRIVIAL (info) << "Looking up local set"
                             << config_.localPrefix
                             << ":"
                             << query_.querystring ()
                             << "."
                             <<  hlv::service::lookup::LOCAL_SET;
    redisAsyncCommand (config_.redisContext,
                       localSmemberCallback,
                       this,
                       "SMEMBERS %s:%s.%s",
                       config_.localPrefix.c_str (),
                       query_.querystring ().c_str (),
                       hlv::service::lookup::LOCAL_SET.c_str ());
}

// Callback for getting PERM bits for local query
void Connection::smemberSucceeded (redisReply* reply) {
    response_.Clear();
    response_.set_token (config_.token);
    response_.set_querystring (query_.querystring ());
    BOOST_LOG_TRIVIAL (info) << "smembers returned";
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements > 0) {
        BOOST_LOG_TRIVIAL (info) << "SMEMBERS succeeded, converting to result";
        // Indicate that we did in fact find a value
        response_.set_success (true);
        for (uint32_t j = 0; j < reply->elements; j ++) {
            auto val = response_.add_values();
            val->set_type ("");
            val->set_value (reply->element[j]->str);
        }
    } else {
        BOOST_LOG_TRIVIAL (info) << "SMEMBERS failed";
        response_.set_success (false);
    }
    query_.Clear ();
    write_response (response_);
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
                    
                    if (!query_.ParseFromArray(buffer_.data(), bytes_transfered)) {
                        BOOST_LOG_TRIVIAL(error) << "Could not parse request";
                        manager_.stop(shared_from_this());
                        return;
                    }
                    
                    if (query_.type () == ev_lookup::Query::GLOBAL) {
                        global_lookup ();
                    } else if (query_.type() == ev_lookup::Query::LOCAL) {
                        local_lookup ();
                    }
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
    response_.Clear ();
    response_.set_token (config_.token);
    response_.set_querystring (query_.querystring ());
    response_.set_success (false);
    query_.Clear ();
    write_response (response_);
}

void Connection::getSucceeded (redisReply* reply) {
    BOOST_LOG_TRIVIAL (info) << "Got response";
    response_.Clear();
    response_.set_token (config_.token);
    response_.set_querystring (query_.querystring ());
    
    // HGETALL responds with an array the where even elements represent
    // hash keys and odd elements represent values
    // See also: http://redis.io/commands/hgetall
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements > 0) {
        // Indicate that we did in fact find a value
        response_.set_success (true);
        for (uint32_t j = 0; j < reply->elements; j += 2) {
            std::string key (reply->element[j]->str);
            if (key == PERM_BIT_FIELD) {
                BOOST_LOG_TRIVIAL (info) << "Checking authorization token"; 
                uint64_t token = std::stoull(std::string
                                            (reply->element[j+1]->str));
                if (token != 0 && !( token & query_.token())) {
                    BOOST_LOG_TRIVIAL (info) << "Not authorized, failing ";
                    response_.set_success (false);
                    response_.clear_values ();
                    break;
                }
            } else {
                auto val = response_.add_values();
                val->set_type (key);
                val->set_value (reply->element[j + 1]->str);
            }
        }
    } else {
        BOOST_LOG_TRIVIAL (info) << "No entry found, failing";
        // Indicate a sad lack of values
        response_.set_success (false);
    }
    query_.Clear ();
    write_response (response_);
}

} // namespace server
} // namespace lookup
} // namespace service
} // namespace hlv
