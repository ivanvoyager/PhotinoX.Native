#include "Photino.Windows.WebView2Environment.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <tuple>

using namespace PhotinoX::Native;

namespace
{
    void NormalizeStringList(std::vector<PlatformString>& values)
    {
        std::ranges::sort(values);

        auto uniqueRange = std::ranges::unique(values);
        values.erase(uniqueRange.begin(), uniqueRange.end());
    }

    auto SchemeTie(const WebView2CustomSchemeKey& key)
    {
        return std::tie(
            key.name,
            key.hasAuthorityComponent,
            key.treatAsSecure,
            key.allowedOrigins);
    }

    void HashCombine(std::size_t& seed, std::size_t value) noexcept
    {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    void HashCombineString(std::size_t& seed, const PlatformString& value) noexcept
    {
        HashCombine(seed, std::hash<PlatformString>{}(value));
    }

    void HashCombineBool(std::size_t& seed, bool value) noexcept
    {
        HashCombine(seed, std::hash<bool>{}(value));
    }

} // namespace

bool PhotinoX::Native::operator==(const WebView2EnvironmentSharingKey& left, const WebView2EnvironmentSharingKey& right) noexcept
{
    return left.runtimePath == right.runtimePath &&
           left.userDataFolder == right.userDataFolder;
}

bool PhotinoX::Native::operator==(const WebView2CustomSchemeKey& left, const WebView2CustomSchemeKey& right) noexcept
{
    return left.name == right.name &&
           left.hasAuthorityComponent == right.hasAuthorityComponent &&
           left.treatAsSecure == right.treatAsSecure &&
           left.allowedOrigins == right.allowedOrigins;
}

bool PhotinoX::Native::operator==(const WebView2EnvironmentKey& left, const WebView2EnvironmentKey& right) noexcept
{
    return left.additionalBrowserArguments == right.additionalBrowserArguments &&
           left.customSchemes == right.customSchemes;
}

void PhotinoX::Native::Normalize(WebView2CustomSchemeKey& key)
{
    NormalizeStringList(key.allowedOrigins);
}

void PhotinoX::Native::Normalize(WebView2EnvironmentKey& key)
{
    for (auto& scheme : key.customSchemes)
        Normalize(scheme);

    std::ranges::sort(
        key.customSchemes,
        [](const WebView2CustomSchemeKey& left, const WebView2CustomSchemeKey& right)
        {
            return SchemeTie(left) < SchemeTie(right);
        });

    auto uniqueRange = std::ranges::unique(key.customSchemes);
    key.customSchemes.erase(uniqueRange.begin(), uniqueRange.end());
}

std::size_t WebView2EnvironmentSharingKeyHash::operator()(const WebView2EnvironmentSharingKey& key) const noexcept
{
    std::size_t seed = 0;

    // Hash is used only to select an unordered_map bucket.
    // Exact key equality is still checked by operator==.
    HashCombineString(seed, key.runtimePath);
    HashCombineString(seed, key.userDataFolder);

    return seed;
}

WebView2EnvironmentCache& WebView2EnvironmentCache::Instance()
{
    static WebView2EnvironmentCache cache;
    return cache;
}

WebView2EnvironmentCache::Result WebView2EnvironmentCache::TryGet(
    const WebView2EnvironmentSharingKey& sharingKey,
    const WebView2EnvironmentKey& environmentKey,
    Microsoft::WRL::ComPtr<ICoreWebView2Environment>& environment)
{
    environment.Reset();

    std::lock_guard lock(mutex_);

    auto it = environments_.find(sharingKey);
    if (it == environments_.end())
        return Result::Miss;

    const WebView2EnvironmentCacheEntry& entry = it->second;

    if (!(entry.environmentKey == environmentKey))
        return Result::Conflict;

    // Same sharing key and same environment key: return the cached environment.
    environment = entry.environment;
    assert(environment);
    return environment ? Result::Hit : Result::Conflict;
}

WebView2EnvironmentCache::Result WebView2EnvironmentCache::Store(
    const WebView2EnvironmentSharingKey& sharingKey,
    const WebView2EnvironmentKey& environmentKey,
    ICoreWebView2Environment* createdEnvironment,
    Microsoft::WRL::ComPtr<ICoreWebView2Environment>& environment)
{
    assert(createdEnvironment);

    // This is an output parameter. Clear any previous value so Conflict returns nullptr.
    environment.Reset();

    if (!createdEnvironment)
        return Result::Conflict;

    std::lock_guard lock(mutex_);

    auto it = environments_.find(sharingKey);
    if (it == environments_.end())
    {
        auto [insertedIt, inserted] = environments_.emplace(
            sharingKey,
            WebView2EnvironmentCacheEntry{
                environmentKey,
                Microsoft::WRL::ComPtr<ICoreWebView2Environment>(createdEnvironment)});
        assert(inserted);

        environment = insertedIt->second.environment;
        return Result::Miss;
    }

    if (!(it->second.environmentKey == environmentKey))
        return Result::Conflict;

    // Same sharing key and same environment key: return the cached environment.
    // The newly created environment is not cached by PhotinoX.
    environment = it->second.environment;
    assert(environment);
    return environment ? Result::Hit : Result::Conflict;
}