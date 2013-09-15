#include <string>
#ifndef __HLV_LOOKUP_CONSTS_H__
#define __HLV_LOOKUP_CONSTS_H__
namespace hlv {
namespace service {
namespace lookup {
    const int32_t SERVER_PORT = 8085;
    const int32_t UPDATE_PORT = 8086;
    const int32_t REDIS_PORT = 6379;
    const std::string REDIS_PREFIX = "ev";
    const std::string PERM_BIT_FIELD = "ev:perm_bits"; 
}
}
}
#endif
