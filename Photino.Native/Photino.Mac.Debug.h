#pragma once

#if defined(__APPLE__) && defined(__OBJC__)

#ifdef PHOTINO_MAC_TRACE
#include <stdio.h>

#define PHOTINO_MAC_LOG(...)          \
    do                                \
    {                                 \
        fprintf(stderr, __VA_ARGS__); \
        fflush(stderr);               \
    } while (0)

#else

#define PHOTINO_MAC_LOG(...) \
    do                       \
    {                        \
    } while (0)

#endif

#endif