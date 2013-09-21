#include <string>
#ifndef __GETIFADDR_H__
#define __GETIFADDR_H__
/// Get the first non loopback address in this machine
/// addr: Return address
/// bool: Success
bool getFirstNonLoopbackAddress (std::string& saddr);
#endif
