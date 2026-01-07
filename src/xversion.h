#ifndef X_VERSION_H
#define X_VERSION_H

#define VERSION_MAJOR 0
#define VERSION_MINOR 5
#define VERSION_PATCH 1
#define VERSION_STRING "0.5.1"

#ifdef __linux
    #define PLATFORM_STRING "linux-x64"
#elif _WIN32
    #define PLATFORM_STRING "windows-x64"
#elif __APPLE__
    #define PLATFORM_STRING "macos-x64"
#else
    #error "Xen is not supported on this platform"
#endif

#define VERSION_STRING_FULL VERSION_STRING "-" PLATFORM_STRING

#endif
