#include "Photino.Export.h"
#include "Photino.Memory.h"

using namespace PhotinoX::Native;

extern "C"
{
    PHOTINO_EXPORT void* Photino_AllocateMemory(int size)
    {
        return AllocateMemory(size);
    }

    PHOTINO_EXPORT void Photino_FreeMemory(void* value)
    {
        FreeMemory(value);
    }

    PHOTINO_EXPORT char* Photino_AllocateString(int size)
    {
        return AllocateString(size);
    }

    PHOTINO_EXPORT void Photino_FreeString(char* value)
    {
        FreeString(value);
    }

    PHOTINO_EXPORT void Photino_FreeStringArray(char** values, int count)
    {
        FreeStringArray(values, count);
    }
}