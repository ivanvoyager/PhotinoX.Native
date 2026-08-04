#pragma once

#ifdef _WIN32

#include "Photino.Strings.h"

#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <WebView2.h>
#include <wrl/client.h>

namespace PhotinoX::Native
{
    // Custom key (runtimePath + userDataFolder) for WebView2EnvironmentCache to be used in unordered_map.
    struct WebView2EnvironmentSharingKey
    {
        PlatformString runtimePath;
        PlatformString userDataFolder;
    };

    struct WebView2CustomSchemeKey
    {
        PlatformString name;
        bool hasAuthorityComponent = true;
        bool treatAsSecure = true;
        std::vector<PlatformString> allowedOrigins;
    };

    struct WebView2EnvironmentKey
    {
        PlatformString additionalBrowserArguments;
        std::vector<WebView2CustomSchemeKey> customSchemes;
    };

    bool operator==(const WebView2EnvironmentSharingKey& left, const WebView2EnvironmentSharingKey& right) noexcept;
    bool operator==(const WebView2CustomSchemeKey& left, const WebView2CustomSchemeKey& right) noexcept;
    bool operator==(const WebView2EnvironmentKey& left, const WebView2EnvironmentKey& right) noexcept;

    void Normalize(WebView2CustomSchemeKey& key);
    void Normalize(WebView2EnvironmentKey& key);

    // Cache entry (environmentKey + environment) for WebView2EnvironmentCache to store the accepted environment config and cached environment.
    struct WebView2EnvironmentCacheEntry
    {
        WebView2EnvironmentKey environmentKey;
        Microsoft::WRL::ComPtr<ICoreWebView2Environment> environment;
    };

    // Hasher (runtimePath + userDataFolder) for WebView2EnvironmentSharingKey to be used in unordered_map.
    struct WebView2EnvironmentSharingKeyHash
    {
        std::size_t operator()(const WebView2EnvironmentSharingKey& key) const noexcept;
    };

    class WebView2EnvironmentCache
    {
      public:
        enum class Result
        {
            Miss,
            Hit,
            Conflict
        };

        static WebView2EnvironmentCache& Instance();

        Result TryGet(
            const WebView2EnvironmentSharingKey& sharingKey,
            const WebView2EnvironmentKey& environmentKey,
            Microsoft::WRL::ComPtr<ICoreWebView2Environment>& environment);

        Result Store(
            const WebView2EnvironmentSharingKey& sharingKey,
            const WebView2EnvironmentKey& environmentKey,
            ICoreWebView2Environment* createdEnvironment,
            Microsoft::WRL::ComPtr<ICoreWebView2Environment>& environment);

      private:
        WebView2EnvironmentCache() = default;

        std::mutex mutex_;
        std::unordered_map<
            WebView2EnvironmentSharingKey,     // Key: browser-process sharing identity (runtimePath + userDataFolder).
            WebView2EnvironmentCacheEntry,     // Value: accepted environment config and cached environment (environmentKey + environment).
            WebView2EnvironmentSharingKeyHash> // Hasher for the custom key (runtimePath + userDataFolder). Equality is operator== via std::equal_to.
            environments_;
    };

} // namespace PhotinoX::Native

#endif