#pragma once

#ifdef _WIN32

#ifdef PHOTINO_WINDOWS_TRACE

#include <cstdio>

#define PHOTINO_WINDOWS_LOG(...)           \
    do                                     \
    {                                      \
        std::fprintf(stderr, __VA_ARGS__); \
        std::fflush(stderr);               \
    } while (0)

#else

#define PHOTINO_WINDOWS_LOG(...) \
    do                           \
    {                            \
    } while (0)

#endif

#endif