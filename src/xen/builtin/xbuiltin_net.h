#ifndef X_BUILTIN_NET_H
#define X_BUILTIN_NET_H

#include "../xcommon.h"
#include "../xvalue.h"

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
#elif defined(__linux__)
    #define PLATFORM_LINUX
#else
    #error "Unsupported platform"
#endif

#ifdef PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")

typedef SOCKET socket_t;
    #define INVALID_SOCKET_FD INVALID_SOCKET
    #define SOCKET_ERROR_CODE WSAGetLastError()
    #define CLOSE_SOCKET closesocket
    #define SHUTDOWN_READ SD_RECEIVE
    #define SHUTDOWN_WRITE SD_SEND
    #define SHUTDOWN_BOTH SD_BOTH
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <errno.h>
    #include <string.h>

typedef int socket_t;
    #define INVALID_SOCKET_FD -1
    #define SOCKET_ERROR_CODE errno
    #define CLOSE_SOCKET close
    #define SHUTDOWN_READ SHUT_RD
    #define SHUTDOWN_WRITE SHUT_WR
    #define SHUTDOWN_BOTH SHUT_RDWR
#endif

xen_obj_namespace* xen_builtin_net();

#endif