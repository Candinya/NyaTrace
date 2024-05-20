#ifndef TRACING_UTILS_H
#define TRACING_UTILS_H

#include "tracing_defs.h"

#ifdef Q_OS_WIN

#include <WS2tcpip.h>

#endif
#ifdef Q_OS_UNIX

// TODO

#endif

addrinfo * ResolveAllAddress(const char * hostname);
bool ParseIPAddress(const char * ipStr, sockaddr_storage & targetIPAddress);
bool PrintIPAddress(sockaddr_storage * targetIPAddress, char * printIPAddress);

#endif // TRACING_UTILS_H
