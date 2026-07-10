#pragma once

namespace PhotinoX::Native
{
    void* AllocateMemory(int size) noexcept;
    void FreeMemory(void* value) noexcept;

    char* AllocateString(int size) noexcept;
    void FreeString(char* value) noexcept;
    void FreeStringArray(char** values, int count) noexcept;
} // namespace PhotinoX::Native