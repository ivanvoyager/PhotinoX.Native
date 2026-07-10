#include "Photino.Memory.h"

#include <cassert>
#include <cstddef>
#include <new>

namespace PhotinoX::Native
{
    void* AllocateMemory(int size) noexcept
    {
        assert(size > 0);

        if (size <= 0)
            return nullptr;

        return new (std::nothrow) unsigned char[static_cast<size_t>(size)];
    }

    void FreeMemory(void* value) noexcept
    {
        delete[] static_cast<unsigned char*>(value);
    }

    char* AllocateString(int size) noexcept
    {
        return static_cast<char*>(AllocateMemory(size));
    }

    void FreeString(char* value) noexcept
    {
        FreeMemory(value);
    }

    void FreeStringArray(char** values, int count) noexcept
    {
        if (!values)
            return;

        for (int i = 0; i < count; i++)
            FreeString(values[i]);

        FreeMemory(values);
    }

} // namespace PhotinoX::Native