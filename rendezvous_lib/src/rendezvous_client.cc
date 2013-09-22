#include <string>
#include <utility>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <consts.h>
#include "rendezvous_client.h"
#include "ebox.pb.h"

namespace ev {
namespace rendezvous {
bool rendezvous_with_other (hlv::lookup::client::EvLookupClient& client,
                            boost::asio::ip::tcp::socket& socket,
                            std::string& service_name,
                            uint64_t token,
                            const std::string& group,
                            uint64_t sessionKey) {
    uint64_t rtoken = 0;
    std::map<std::string, std::string> results;
    client.Query (token, 
                  service_name,
                  rtoken,
                  results);
    auto box = results.find (hlv::service::lookup::RENDEZVOUS_LOCATION);
    if (box == results.end ()) {
        return false;
    }

    auto box_addr = box->second;
    std::vector<std::string> split_results;

    // Break address into address port
    boost::split(split_results, box_addr, boost::is_any_of(":"), boost::token_compress_off);
    if (split_results.size () != 2) {
        std::cerr << "Do not understand response" << std::endl;
        return false;
    }
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({split_results[0], split_results[1]});
    boost::system::error_code ec;
    socket.connect(endpoint, ec);
    if (ec) {
        BOOST_LOG_TRIVIAL(error) << "Error connecting to remote endpoint";
        return false;
    }
    std::array<char, 131072> buffer;
    ev_ebox::RegisterSession reg;
    reg.set_token (token);
    reg.set_group (group);
    reg.set_sessionkey (sessionKey);
    uint64_t size = reg.ByteSize ();
    *(buffer.data ()) = size;
    reg.SerializeToArray (buffer.data() + sizeof(uint64_t), size);
    boost::asio::write (socket,
      boost::asio::buffer(buffer),
      boost::asio::transfer_exactly (size + sizeof(uint64_t)),
      ec);
    if (ec) {
        BOOST_LOG_TRIVIAL (error) << "Failed to register";
        return false;
    }
    return true;
}
}
}
