#include <string>
#ifndef __HLV_LOOKUP_CONSTS_H__
#define __HLV_LOOKUP_CONSTS_H__
namespace hlv {
namespace service {
namespace lookup {
/// A set of constants
    const int32_t SERVER_PORT = 8085;
    const int32_t UPDATE_PORT = 8086;
    const int32_t EBOX_PORT   = 8087;
    const int32_t REDIS_PORT = 6379;
    const std::string REDIS_PREFIX = "ev";
    const std::string PERM_BIT_FIELD = "ev:perm_bits"; 
    const std::string LOCAL_SET = "ev:local_set";
    const int32_t AUTH_PORT = 8087;
    const std::string AUTH_LOCATION = "auth";
    const std::string LDEBOX_LOCATION = "ev:ebox";
    const std::string PROVIDER_LOCATION = "ev:provider";
}
}
}
#endif
