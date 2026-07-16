#include "Photino.h"

#ifdef _WIN32
#include <cwchar>
#else
#include <strings.h>
#endif

#include <utility>

using namespace PhotinoX::Native;

bool Photino::IsCustomSchemeRegistered(const PlatformString& scheme) const
{
    if (scheme.empty())
        return false;

#ifdef _WIN32
    for (const auto& existing : customSchemeNames_)
    {
        if (_wcsicmp(existing.c_str(), scheme.c_str()) == 0)
            return true;
    }
#else
    for (const auto& existing : customSchemeNames_)
    {
        if (strcasecmp(existing.c_str(), scheme.c_str()) == 0)
            return true;
    }
#endif

    return false;
}

bool Photino::AddCustomSchemeName(Utf8String scheme)
{
    if (!scheme || *scheme == '\0') return false;

    PlatformString nativeScheme = ToPlatformString(scheme);
    if (nativeScheme.empty()) return false;

    if (IsCustomSchemeRegistered(nativeScheme)) return true;

    if (customSchemeNames_.size() >= MaxCustomSchemeNames) return false;

    customSchemeNames_.emplace_back(std::move(nativeScheme));

    return RegisterCustomSchemeName(customSchemeNames_.back());
}