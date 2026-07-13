#pragma once

#include <string>
#include <vector>

#if defined(__APPLE__) && defined(__OBJC__)
#import <Foundation/Foundation.h>
#endif

namespace PhotinoX::Native
{
    // ABI string received from .NET/PInvoke.
    // Non-owning, null-terminated UTF-8 pointer.
    // Do not store this pointer. Convert/copy to PlatformString first.
    using Utf8String = const char*;
#ifdef _WIN32
    // Owned internal string used with Win32/WebView2 APIs. Content is UTF-16
    using PlatformString = std::wstring;
#else
    // Owned internal string used with GTK/WebKit/macOS bridge. Content is UTF-8.
    using PlatformString = std::string;
#endif

    PlatformString ToPlatformString(Utf8String source);
    std::string ToUtf8String(const PlatformString& source);
    char* CopyUtf8String(const std::string& value);
    std::vector<PlatformString> ToPlatformStringList(Utf8String* values, int count);
    char** CopyUtf8StringArray(const std::vector<PlatformString>& values, int* resultCount);

#if defined(__APPLE__) && defined(__OBJC__)
    inline NSString* ToNSString(const PlatformString& value)
    {
        return [NSString stringWithUTF8String:value.c_str()];
    }
#endif

} // namespace PhotinoX::Native