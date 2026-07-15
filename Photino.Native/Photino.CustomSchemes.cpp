#include "Photino.h"

#ifdef _WIN32
#include <cwchar>
#else
#include <strings.h>
#endif

using namespace PhotinoX::Native;

bool Photino::IsCustomScheme(const PlatformString& scheme) const
{
    if (scheme.empty())
        return false;

#ifdef _WIN32
    for (const auto& existing : _customSchemeNames)
    {
        if (_wcsicmp(existing.c_str(), scheme.c_str()) == 0)
            return true;
    }
#else
    for (const auto& existing : _customSchemeNames)
    {
        if (strcasecmp(existing.c_str(), scheme.c_str()) == 0)
            return true;
    }
#endif

    return false;
}