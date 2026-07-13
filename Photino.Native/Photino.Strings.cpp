#include "Photino.Strings.h"
#include "Photino.Memory.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <cstring>

namespace PhotinoX::Native
{
    PlatformString ToPlatformString(Utf8String source)
    {
        if (!source)
            return {};
#ifdef _WIN32
        int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source, -1, nullptr, 0);
        if (required <= 0)
            return {};

        std::wstring result(static_cast<size_t>(required), L'\0');

        int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source, -1, result.data(), required);
        if (written <= 0)
            return {};

        result.resize(static_cast<size_t>(written - 1)); // remove terminating '\0'
        return result;
#else
        return PlatformString(source);
#endif
    }

    std::string ToUtf8String(const PlatformString& source)
    {
#ifdef _WIN32
        if (source.empty())
            return {};

        int required = WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            source.c_str(),
            -1,
            nullptr,
            0,
            nullptr,
            nullptr);

        if (required <= 0)
            return {};

        std::string result(static_cast<size_t>(required), '\0');

        int written = WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            source.c_str(),
            -1,
            result.data(),
            required,
            nullptr,
            nullptr);

        if (written <= 0)
            return {};

        result.resize(static_cast<size_t>(written - 1)); // remove terminating '\0'
        return result;
#else
        return source;
#endif
    }

    char* CopyUtf8String(const std::string& value)
    {
        auto result = AllocateString(static_cast<int>(value.size() + 1));
        if (!result)
            return nullptr;

        std::memcpy(result, value.c_str(), value.size() + 1);
        return result;
    }

    std::vector<PlatformString> ToPlatformStringList(Utf8String* values, int count)
    {
        std::vector<PlatformString> result;

        if (!values || count <= 0)
            return result;

        result.reserve(static_cast<size_t>(count));

        for (int i = 0; i < count; i++)
        {
            if (values[i])
                result.emplace_back(ToPlatformString(values[i]));
        }

        return result;
    }

    char** CopyUtf8StringArray(const std::vector<PlatformString>& values, int* resultCount)
    {
        if (!resultCount)
            return nullptr;

        *resultCount = static_cast<int>(values.size());

        if (values.empty())
            return nullptr;

        auto result = static_cast<char**>(AllocateMemory(static_cast<int>(sizeof(char*) * values.size())));
        if (!result)
        {
            *resultCount = 0;
            return nullptr;
        }

        for (size_t i = 0; i < values.size(); i++)
            result[i] = CopyUtf8String(ToUtf8String(values[i]));

        return result;
    }
}