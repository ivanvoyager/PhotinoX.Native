#pragma once

#ifdef __linux__

#ifdef PHOTINO_LINUX_TRACE
#include <stdio.h>

#define PHOTINO_LINUX_LOG(...)        \
    do                                \
    {                                 \
        fprintf(stderr, __VA_ARGS__); \
        fflush(stderr);               \
    } while (0)

#else

#define PHOTINO_LINUX_LOG(...) \
    do                         \
    {                          \
    } while (0)

#endif

#endif