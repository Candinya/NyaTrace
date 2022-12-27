#ifndef TRACING_UTILS_H
#define TRACING_UTILS_H

#include <WS2tcpip.h>
#include "tracing_defs.h"

addrinfo * ResolveAllAddress(const char * hostname);
bool ParseIPAddress(const char * ipStr, sockaddr_storage & targetIPAddress);
bool PrintIPAddress(sockaddr_storage * targetIPAddress, char * printIPAddress);

#endif // TRACING_UTILS_H
