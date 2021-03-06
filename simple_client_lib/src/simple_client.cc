#include <string>
#include <utility>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include "simple_client.h"
#include "consts.h"
namespace hlv {
namespace simple {
namespace client {
/// Construct an EV Lookup Client.
/// host; string address of lookup host
/// port: uint32_t port of lookup host
EvSimpleClient::EvSimpleClient (const std::string name,
                         const std::string lname,
                         const uint64_t token,
                         hlv::lookup::client::EvLookupClient&  client, 
                         std::unique_ptr<hlv::lookup::client::EvLookupClient> localClient):
                 servicename_ (name),
                 lname_ (lname),
                 token_ (token),
                 client_ (client),
                 localClient_ (std::move(localClient)),
                 io_service_ () {
}

// send a query to the server
bool EvSimpleClient::send_query (const std::string& query,
                                boost::asio::ip::tcp::socket& socket) const {
    uint64_t size = query.size ();
    *((uint64_t*)buffer_.data()) = size;
    query.copy (buffer_.data() + sizeof(uint64_t), std::string::npos);
    BOOST_LOG_TRIVIAL (info) << "sending query";
    boost::system::error_code ec;
    boost::asio::write (socket,
      boost::asio::buffer(buffer_),
      boost::asio::transfer_exactly (size + sizeof(uint64_t)),
      ec);
    if (ec) {
        BOOST_LOG_TRIVIAL (info) << "error sending query " << ec;
        return false;
    }
    BOOST_LOG_TRIVIAL (info) << "succeeded in sending query";
    return true;
}

// send a query to echo server
bool EvSimpleClient::send_query_echo (const std::string& query,
                                      std::string& response,
                                boost::asio::ip::tcp::socket& socket) const {
    uint64_t size = query.size ();
    BOOST_LOG_TRIVIAL (info) << "sending echo";
    boost::system::error_code ec;
    boost::asio::write (socket,
      boost::asio::buffer(query),
      boost::asio::transfer_exactly (size),
      ec);
    if (ec) {
        BOOST_LOG_TRIVIAL (info) << "error sending query " << ec;
        return false;
    }
    BOOST_LOG_TRIVIAL (info) << "succeeded in sending query";
    size_t reply_length = socket.read_some(
                                            boost::asio::buffer (buffer_), 
                                            ec);
    if (ec) {
        BOOST_LOG_TRIVIAL (info) << "error reading response " << ec;
        return false;
    }
    response = std::string(buffer_.data(), reply_length);
    return true;
}

bool EvSimpleClient::echo_request (const std::string& type, const std::string& str, std::string& response) {
    uint64_t token = 0;
    std::map<std::string, std::string> results;
    client_.Query (token_,
                   servicename_,
                   token,
                   results);
    auto server = results.find (type);
    if (server == results.end ()) {
        BOOST_LOG_TRIVIAL (info) << "Could not find provider";
        return false;
    }
    std::vector<std::string> split_results;
    boost::split(split_results, server->second, boost::is_any_of(":"), boost::token_compress_off);
    
    if (split_results.size() != 2) {
        BOOST_LOG_TRIVIAL (info) << "Do not understand return";
        return false;
    }

    boost::asio::ip::tcp::socket sock (io_service_);
    boost::asio::ip::tcp::resolver resolver(io_service_);


    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({split_results[0], split_results[1]});
    boost::system::error_code ec;
    sock.connect(endpoint, ec);
    if (ec) {
        BOOST_LOG_TRIVIAL(error) << "Error connecting to remote endpoint";
        return false;
    }
    return send_query_echo(str, response, sock);
}

bool EvSimpleClient::local_echo_request (const std::string& type, const std::string& str, std::string& response) {
    uint64_t token = 0;
    std::list<std::string> results;
    client_.LocalQuery (token_,
                        lname_,
                        token,
                        results);
    for (auto addr : results) {
        std::vector<std::string> split_results;
        boost::split(split_results, addr, boost::is_any_of(":"), boost::token_compress_off);
        
        if (split_results.size() != 2) {
            BOOST_LOG_TRIVIAL (info) << "Do not understand return";
            return false;
        }

        boost::asio::ip::tcp::socket sock (io_service_);
        boost::asio::ip::tcp::resolver resolver(io_service_);


        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({split_results[0], split_results[1]});
        boost::system::error_code ec;
        sock.connect(endpoint, ec);
        if (ec) {
            BOOST_LOG_TRIVIAL(error) << "Error connecting to remote endpoint " << addr;
        }
        send_query_echo(str, response, sock);
    }
    return true;
}

bool EvSimpleClient::send_to_servers (const std::string& type, const std::string& str) {
    uint64_t token = 0;
    std::map<std::string, std::string> results;
    client_.Query (token_,
                   servicename_,
                   token,
                   results);
    auto server = results.find (type);
    if (server == results.end ()) {
        BOOST_LOG_TRIVIAL (info) << "Could not find provider";
        return false;
    }
    std::vector<std::string> split_results;
    boost::split(split_results, server->second, boost::is_any_of(":"), boost::token_compress_off);
    
    if (split_results.size() != 2) {
        BOOST_LOG_TRIVIAL (info) << "Do not understand return";
        return false;
    }

    boost::asio::ip::tcp::socket sock (io_service_);
    boost::asio::ip::tcp::resolver resolver(io_service_);


    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({split_results[0], split_results[1]});
    boost::system::error_code ec;
    sock.connect(endpoint, ec);
    if (ec) {
        BOOST_LOG_TRIVIAL(error) << "Error connecting to remote endpoint";
        return false;
    }
    return send_query(str, sock);
}

bool EvSimpleClient::send_to_servers (const std::string& str) {
    return send_to_servers (hlv::service::lookup::PROVIDER_LOCATION, str);
}

bool EvSimpleClient::send_everywhere (const std::string& str) {
    return send_everywhere (hlv::service::lookup::PROVIDER_LOCATION, str);
}

bool EvSimpleClient::send_everywhere (const std::string& type, const std::string& str) {
    std::list<std::string> results;
    uint64_t token;
    localClient_->LocalQuery (token_,
                        lname_,
                        token,
                        results);
    for (auto addr : results) {
        std::vector<std::string> split_results;
        boost::split(split_results, addr, boost::is_any_of(":"), boost::token_compress_off);
        
        if (split_results.size() != 2) {
            BOOST_LOG_TRIVIAL (info) << "Do not understand return";
            continue;
        }

        boost::asio::ip::tcp::socket sock (io_service_);
        boost::asio::ip::tcp::resolver resolver(io_service_);


        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({split_results[0], split_results[1]});
        boost::system::error_code ec;
        sock.connect(endpoint, ec);
        if (ec) {
            BOOST_LOG_TRIVIAL(error) << "Error connecting to remote endpoint " << addr;
            continue;
        }
        send_query (str, sock);
    }
    return send_to_servers (type, str);
}

EvSimpleClient::~EvSimpleClient() {
}
}
}
}
