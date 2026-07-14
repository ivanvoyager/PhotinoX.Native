#pragma once

#ifdef _WIN32
#define PHOTINO_EXPORT __declspec(dllexport)
#else
#define PHOTINO_EXPORT
#endif