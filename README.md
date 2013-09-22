Extended Virtualization
=======================

This is code for the SDNv2 Extended Virtualization component. The interesting reusable parts
are largely in the misc directory. Here is a rundown of the contents

asiohiredis: ASIO adapter for hiredis ([github.com/redis/hiredis])

auth: A simple (trivial) authentication server.

cmake\_components: Some CMake modules. So far these find hiredis

common: Old files, mainly contains templates to set up Boost ASIO based servers in a jiffy.

coordinator: An update coordinator (see the EV documentation/paper for uses).

coordinator\_client\_example: Example client of coordinator, this was just test code

coordinator\_library: A library used by applications which use the coordinator

dropbox\_client: The main demo EV application

ev\_lookup\_client\_async\_library: Dragons live here. Actually at one point I wanted an async library, this one works pretty reasonably.

ev\_lookup\_client\_example: Like dig for the EV lookup service

ev\_lookup\_library: Application support for lookup

ev\_lookup\_server: The EV lookup server, an extended DNS service supporint authentication

include: Some random constants

ldiscovery\_client\_lib: A library to talk to the local discovery edge box; a mechanism to change lookup bindings for the local edge.

ldiscovery\_edge\_box: A server that changes local lookup binding.

misc: Miscellaneous libraries including some code to list network interfaces and linenoise ([github.com/antirez/linenoise]) usable in C++

proto: Protobuf files

simple\_client\_lib: Just factored out client code to talk to the simple service

simple\_server: A dead simple server that prints whatever it is sent.

simple\_server\_lib: Most of the code for the simple server.

Dependencies:
    OpenSSL
    CMake
    Boost 1.54
    Protobuf
    HiRedis
