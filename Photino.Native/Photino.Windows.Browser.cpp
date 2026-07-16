#include "Photino.h"
#include "Photino.Memory.h"
#include "Photino.Strings.h"
#include "Photino.Windows.State.h"

#include <WebView2.h>
#include <Shlwapi.h>
#include <WebView2EnvironmentOptions.h>
#include <Windows.h>
#include <wrl.h>
#include <comdef.h>

#include <algorithm>
#include <cassert>

#pragma comment(lib, "Urlmon.lib")

using namespace Microsoft::WRL;
using namespace PhotinoX::Native;

PlatformString g_webview2RuntimePath;

void Photino::GetTransparentEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.transparentEnabled;

    if (!platform_->webViewController) return;

    wil::com_ptr<ICoreWebView2Controller2> controller2;
    if (FAILED(platform_->webViewController->QueryInterface(&controller2)) || !controller2) return;

    COREWEBVIEW2_COLOR backgroundColor{};
    if (SUCCEEDED(controller2->get_DefaultBackgroundColor(&backgroundColor)))
        *enabled = (backgroundColor.A == 0);
}

void Photino::SetTransparentEnabled(const bool enabled)
{
    options_.transparentEnabled = enabled;

    assert(platform_->webViewController && platform_->webViewWindow);
    if (!platform_->webViewController || !platform_->webViewWindow) return;

    wil::com_ptr<ICoreWebView2Controller2> controller2;
    if (FAILED(platform_->webViewController->QueryInterface(&controller2)) || !controller2) return;

    COREWEBVIEW2_COLOR backgroundColor{};
    HRESULT hr = controller2->get_DefaultBackgroundColor(&backgroundColor);
    if (SUCCEEDED(hr))
    {
        backgroundColor.A = enabled ? 0 : 255;
        hr = controller2->put_DefaultBackgroundColor(backgroundColor);
        assert(SUCCEEDED(hr));
    }

    hr = platform_->webViewWindow->Reload();
    assert(SUCCEEDED(hr));
}

void Photino::ClearBrowserAutoFill() const
{
    if (!platform_->webViewWindow) return;

    auto webview15 = platform_->webViewWindow.try_query<ICoreWebView2_15>();
    if (!webview15)
        return;

    wil::com_ptr<ICoreWebView2Profile> profile;
    HRESULT hr = webview15->get_Profile(&profile);
    if (FAILED(hr) || !profile)
        return;

    auto profile2 = profile.try_query<ICoreWebView2Profile2>();
    if (!profile2)
        return;

    COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds =
        COREWEBVIEW2_BROWSING_DATA_KINDS_GENERAL_AUTOFILL |
        COREWEBVIEW2_BROWSING_DATA_KINDS_PASSWORD_AUTOSAVE;

    hr = profile2->ClearBrowsingData(dataKinds,
                                     Callback<ICoreWebView2ClearBrowsingDataCompletedHandler>(
                                         [](HRESULT error) -> HRESULT
                                         {
                                             return S_OK;
                                         })
                                         .Get());
    assert(SUCCEEDED(hr));
}

/*
 * The htmlContent parameter may not be larger than 2 MB (2 * 1024 * 1024 bytes) in total size.
 * The origin of the new page is about:blank.
 */
void Photino::NavigateToString(const PlatformString& content) const
{
    assert(platform_->webViewWindow);
    if (!platform_->webViewWindow) return;

    HRESULT hr = platform_->webViewWindow->NavigateToString(content.c_str());
    assert(SUCCEEDED(hr));
}

void Photino::NavigateToUrl(const PlatformString& url) const
{
    assert(platform_->webViewWindow);
    if (!platform_->webViewWindow || url.empty()) return;

    HRESULT hr = platform_->webViewWindow->Navigate(url.c_str());
    assert(SUCCEEDED(hr));
}

void Photino::SendWebMessage(const PlatformString& message) const
{
    assert(platform_->webViewWindow);
    if (!platform_->webViewWindow) return;

    HRESULT hr = platform_->webViewWindow->PostWebMessageAsString(message.c_str());
    assert(SUCCEEDED(hr));
}

void Photino::GetContextMenuEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.contextMenuEnabled;

    if (!platform_->webViewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(platform_->webViewWindow->get_Settings(&settings)) || !settings) return;

    BOOL value = FALSE;
    if (SUCCEEDED(settings->get_AreDefaultContextMenusEnabled(&value)))
        *enabled = (value == TRUE);
}

void Photino::SetContextMenuEnabled(const bool enabled)
{
    options_.contextMenuEnabled = enabled;
    if (!platform_->webViewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(platform_->webViewWindow->get_Settings(&settings)) || !settings) return;

    HRESULT hr = settings->put_AreDefaultContextMenusEnabled(enabled ? TRUE : FALSE);
    assert(SUCCEEDED(hr));

    hr = platform_->webViewWindow->Reload();
    assert(SUCCEEDED(hr));
}

void Photino::GetZoomEnabled(bool* enabled) const
{
    assert(enabled && platform_->webViewWindow);
    if (!enabled) return;

    *enabled = options_.zoomEnabled;

    if (!platform_->webViewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(platform_->webViewWindow->get_Settings(&settings)) || !settings) return;

    BOOL value = FALSE;
    if (SUCCEEDED(settings->get_IsZoomControlEnabled(&value)))
        *enabled = (value == TRUE);
}

void Photino::SetZoomEnabled(const bool enabled)
{
    options_.zoomEnabled = enabled;

    assert(platform_->webViewWindow);
    if (!platform_->webViewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(platform_->webViewWindow->get_Settings(&settings)) || !settings) return;

    HRESULT hr = settings->put_IsZoomControlEnabled(enabled ? TRUE : FALSE);
    assert(SUCCEEDED(hr));

    hr = platform_->webViewWindow->Reload();
    assert(SUCCEEDED(hr));
}

void Photino::GetZoom(int* zoom) const
{
    assert(zoom);
    if (!zoom) return;

    *zoom = options_.zoom;

    if (!platform_->webViewController) return;

    double rawValue = 0.0;
    if (FAILED(platform_->webViewController->get_ZoomFactor(&rawValue))) return;

    rawValue = (rawValue * 100.0) + 0.5; // rounding
    *zoom = static_cast<int>(rawValue);
}

void Photino::SetZoom(int zoom)
{
    if (zoom < 25)
        zoom = 25;
    else if (zoom > 500)
        zoom = 500;

    options_.zoom = zoom;

    assert(platform_->webViewController);
    if (!platform_->webViewController) return;

    double newZoom = static_cast<double>(zoom) / 100.0;
    HRESULT hr = platform_->webViewController->put_ZoomFactor(newZoom);
    assert(SUCCEEDED(hr));
}

void Photino::GetDevToolsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.devToolsEnabled;

    if (!platform_->webViewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(platform_->webViewWindow->get_Settings(&settings)) || !settings) return;

    BOOL value = FALSE;
    if (SUCCEEDED(settings->get_AreDevToolsEnabled(&value)))
        *enabled = (value == TRUE);
}

void Photino::SetDevToolsEnabled(const bool enabled)
{
    options_.devToolsEnabled = enabled;

    if (!platform_->webViewWindow) return;

    wil::com_ptr<ICoreWebView2Settings> settings;
    if (FAILED(platform_->webViewWindow->get_Settings(&settings)) || !settings) return;

    HRESULT hr = settings->put_AreDevToolsEnabled(enabled ? TRUE : FALSE);
    if (FAILED(hr)) return;

    hr = platform_->webViewWindow->Reload();
    assert(SUCCEEDED(hr));
}

void Photino::GetGrantBrowserPermissions(bool* grant) const
{
    assert(grant);
    if (!grant) return;

    *grant = options_.grantBrowserPermissions;
}

void Photino::GetMediaAutoplayEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.mediaAutoplayEnabled;
}

void Photino::GetFileSystemAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.fileSystemAccessEnabled;
}

void Photino::GetWebSecurityEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.webSecurityEnabled;
}

void Photino::GetJavascriptClipboardAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.javascriptClipboardAccessEnabled;
}

void Photino::GetMediaStreamEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.mediaStreamEnabled;
}

void Photino::GetSmoothScrollingEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.smoothScrollingEnabled;
}

void Photino::GetIgnoreCertificateErrorsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = options_.ignoreCertificateErrorsEnabled;
}

PlatformString Photino::BuildStartupString() const
{
    // TODO: Implement special startup strings.
    // https://peter.sh/experiments/chromium-command-line-switches/
    // https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2environmentoptions.additionalbrowserarguments?view=webview2-dotnet-1.0.1938.49&viewFallbackFrom=webview2-dotnet-1.0.1901.177view%3Dwebview2-1.0.1901.177
    // https://www.chromium.org/developers/how-tos/run-chromium-with-flags/
    // Add together all 7 special startup strings, plus the generic one passed by the user to make one big string. Try not to duplicate anything. Separate with spaces.

    PlatformString startupString;

    if (!options_.userAgent.empty())
    {
        PlatformString userAgent = options_.userAgent;
        std::ranges::replace(userAgent, L'"', L'\'');
        startupString += L"--user-agent=\"" + userAgent + L"\" ";
    }

    if (options_.mediaAutoplayEnabled)
        startupString += L"--autoplay-policy=no-user-gesture-required ";

    if (options_.fileSystemAccessEnabled)
        startupString += L"--allow-file-access-from-files ";

    if (!options_.webSecurityEnabled)
        startupString += L"--disable-web-security ";

    if (options_.javascriptClipboardAccessEnabled)
        startupString += L"--enable-javascript-clipboard-access ";

    if (options_.mediaStreamEnabled)
        startupString += L"--enable-usermedia-screen-capturing ";

    if (!options_.smoothScrollingEnabled)
        startupString += L"--disable-smooth-scrolling ";

    if (options_.ignoreCertificateErrorsEnabled)
        startupString += L"--ignore-certificate-errors ";

    if (!options_.browserControlInitParameters.empty())
    {
        if (!startupString.empty() && startupString.back() != L' ')
            startupString += L' ';
        startupString += options_.browserControlInitParameters; // e.g.--hide-scrollbars
    }

    return startupString;
}

HRESULT Photino::CompleteWebViewInitialization()
{
    assert(!platform_->webViewInitialized);
    if (platform_->webViewInitialized)
        return S_OK;

    platform_->webViewInitialized = true;

    if (!options_.startUrl.empty())
    {
        NavigateToUrl(options_.startUrl);
    }
    else if (!options_.startString.empty())
    {
        NavigateToString(options_.startString);
    }
    else
    {
        MessageBoxW(nullptr, L"Neither StartUrl nor StartString was specified", L"Native Initialization Failed", MB_OK);
        std::abort();
    }

    if (options_.contextMenuEnabled == false)
        SetContextMenuEnabled(false);

    if (options_.zoomEnabled == false)
        SetZoomEnabled(false);

    if (options_.devToolsEnabled == false)
        SetDevToolsEnabled(false);

    if (options_.transparentEnabled == true)
        SetTransparentEnabled(true);

    if (options_.zoom != 100)
        SetZoom(options_.zoom);

    RefitContent();
    FocusWebView2();

    return S_OK;
}

HRESULT Photino::HandleScriptAddedOnDocumentCreated(HRESULT result, LPCWSTR id)
{
    if (FAILED(result)) return result;

    platform_->scriptId = id ? id : L"";

    return CompleteWebViewInitialization();
}

HRESULT Photino::HandleWebMessageReceived(ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args)
{
    if (!args) return E_POINTER;
    if (!webMessageReceivedCallback_) return S_OK;

    wil::unique_cotaskmem_string message;
    HRESULT hr = args->TryGetWebMessageAsString(&message);
    if (FAILED(hr))
        return hr;

    wil::unique_cotaskmem_string sourceUri;
    hr = args->get_Source(&sourceUri);
    if (FAILED(hr))
        return hr;

    std::string utf8Message = ToUtf8String(message ? PlatformString(message.get()) : PlatformString());
    webMessageReceivedCallback_(utf8Message.c_str());

    return S_OK;
}

HRESULT Photino::HandleWebResourceRequested(ICoreWebView2* webview, ICoreWebView2WebResourceRequestedEventArgs* args)
{
    if (!args) return E_POINTER;

    wil::com_ptr<ICoreWebView2WebResourceRequest> request;
    HRESULT hr = args->get_Request(&request);
    if (FAILED(hr)) return hr;
    if (!request) return E_POINTER;

    wil::unique_cotaskmem_string uri;
    hr = request->get_Uri(&uri);
    if (FAILED(hr)) return hr;
    if (!uri) return E_POINTER;

    PlatformString uriString(uri.get());
    size_t colonPos = uriString.find(L':');

    if (colonPos == PlatformString::npos || colonPos == 0)
        return S_OK;

    PlatformString scheme = uriString.substr(0, colonPos);

    if (!customSchemeCallback_ || !IsCustomSchemeRegistered(scheme))
        return S_OK;

    std::string uriUtf8 = ToUtf8String(uriString);

    int numBytes = 0;
    Utf8String contentType = nullptr;
    void* responseData = customSchemeCallback_(uriUtf8.c_str(), &numBytes, &contentType);

    if (!platform_->webViewEnvironment)
    {
        FreeMemory(responseData);
        FreeString(const_cast<char*>(contentType));
        return E_POINTER;
    }

    HRESULT responseResult = S_OK;
    if (!responseData || numBytes <= 0)
    {
        wil::com_ptr<IStream> emptyStream;
        emptyStream.attach(SHCreateMemStream(nullptr, 0));

        if (!emptyStream)
        {
            FreeMemory(responseData);
            FreeString(const_cast<char*>(contentType));
            return E_OUTOFMEMORY;
        }

        wil::com_ptr<ICoreWebView2WebResourceResponse> response;
        responseResult = platform_->webViewEnvironment->CreateWebResourceResponse(
            emptyStream.get(),
            404,
            L"Not Found",
            nullptr,
            &response);

        if (SUCCEEDED(responseResult))
            responseResult = args->put_Response(response.get());
    }
    else
    {
        PlatformString headers;

        if (contentType && *contentType)
            headers = L"Content-Type: " + ToPlatformString(contentType);

        wil::com_ptr<IStream> dataStream;
        dataStream.attach(SHCreateMemStream(static_cast<const BYTE*>(responseData), static_cast<UINT>(numBytes)));

        if (dataStream)
        {
            wil::com_ptr<ICoreWebView2WebResourceResponse> response;
            responseResult = platform_->webViewEnvironment->CreateWebResourceResponse(
                dataStream.get(),
                200,
                L"OK",
                headers.empty() ? nullptr : headers.c_str(),
                &response);

            if (SUCCEEDED(responseResult))
                responseResult = args->put_Response(response.get());
        }
        else
        {
            responseResult = E_OUTOFMEMORY;
        }
    }

    FreeMemory(responseData);
    FreeString(const_cast<char*>(contentType));

    return responseResult;
}

HRESULT Photino::HandlePermissionRequested(ICoreWebView2* webview, ICoreWebView2PermissionRequestedEventArgs* args)
{
    if (!args) return E_POINTER;

    if (options_.grantBrowserPermissions)
        return args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);

    return S_OK;
}

HRESULT Photino::HandleWebViewControllerCreated(HRESULT result, ICoreWebView2Controller* controller)
{
    if (FAILED(result)) return result;
    if (!controller) return E_POINTER;

    HRESULT hr = controller->QueryInterface(&platform_->webViewController);
    if (FAILED(hr)) return hr;

    hr = platform_->webViewController->get_CoreWebView2(&platform_->webViewWindow);
    if (FAILED(hr)) return hr;
    if (!platform_->webViewWindow) return E_POINTER;

    wil::com_ptr<ICoreWebView2Settings> settings;
    hr = platform_->webViewWindow->get_Settings(&settings);
    if (FAILED(hr)) return hr;
    if (!settings) return E_POINTER;

    hr = settings->put_AreHostObjectsAllowed(TRUE);
    if (FAILED(hr)) return hr;
    hr = settings->put_IsScriptEnabled(TRUE);
    if (FAILED(hr)) return hr;
    hr = settings->put_AreDefaultScriptDialogsEnabled(TRUE);
    if (FAILED(hr)) return hr;
    hr = settings->put_IsWebMessageEnabled(TRUE);
    if (FAILED(hr)) return hr;

    EventRegistrationToken webMessageToken;
    hr = platform_->webViewWindow->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(this, &Photino::HandleWebMessageReceived)
            .Get(),
        &webMessageToken);
    if (FAILED(hr)) return hr;

    hr = platform_->webViewWindow->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
    if (FAILED(hr)) return hr;

    EventRegistrationToken webResourceRequestedToken;
    hr = platform_->webViewWindow->add_WebResourceRequested(
        Callback<ICoreWebView2WebResourceRequestedEventHandler>(this, &Photino::HandleWebResourceRequested)
            .Get(),
        &webResourceRequestedToken);
    if (FAILED(hr)) return hr;

    EventRegistrationToken permissionRequestedToken;
    hr = platform_->webViewWindow->add_PermissionRequested(
        Callback<ICoreWebView2PermissionRequestedEventHandler>(this, &Photino::HandlePermissionRequested)
            .Get(),
        &permissionRequestedToken);
    if (FAILED(hr)) return hr;

    hr = platform_->webViewWindow->AddScriptToExecuteOnDocumentCreated(
        L"window.external = { sendMessage: function(message) { window.chrome.webview.postMessage(message); }, receiveMessage: function(callback) { window.chrome.webview.addEventListener('message', function(e) { callback(e.data); }); } };",
        Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(this, &Photino::HandleScriptAddedOnDocumentCreated)
            .Get());
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT Photino::HandleWebViewEnvironmentCreated(HRESULT result, ICoreWebView2Environment* environment)
{
    if (FAILED(result)) return result;
    if (!environment) return E_POINTER;

    HRESULT hr = environment->QueryInterface(&platform_->webViewEnvironment);
    if (FAILED(hr)) return hr;

    return environment->CreateCoreWebView2Controller(platform_->hWnd,
                                                     Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(this, &Photino::HandleWebViewControllerCreated)
                                                         .Get());
}

void Photino::AttachWebView()
{
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    if (!options)
    {
        MessageBoxW(platform_->hWnd, L"Failed to allocate WebView2 environment options.", L"Error configuring webview", MB_OK);
        return;
    }

    PlatformString startupString = BuildStartupString();
    if (!startupString.empty())
    {
        HRESULT hr = options->put_AdditionalBrowserArguments(startupString.c_str());
        if (FAILED(hr))
        {
            _com_error err(hr);
            MessageBoxW(platform_->hWnd, err.ErrorMessage(), L"Error configuring webview", MB_OK);
            return;
        }
    }

    PCWSTR runtimePath = g_webview2RuntimePath.empty() ? nullptr : g_webview2RuntimePath.c_str();
    PCWSTR userDataFolder = options_.temporaryFilesPath.empty() ? nullptr : options_.temporaryFilesPath.c_str();

    HRESULT envResult = CreateCoreWebView2EnvironmentWithOptions(runtimePath, userDataFolder, options.Get(),
                                                                 Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(this, &Photino::HandleWebViewEnvironmentCreated)
                                                                     .Get());

    if (FAILED(envResult))
    {
        _com_error err(envResult);
        LPCTSTR errMsg = err.ErrorMessage();
        MessageBoxW(platform_->hWnd, errMsg, L"Error instantiating webview", MB_OK);
    }
}

void Photino::RefitContent() const
{
    if (!platform_->webViewController || !platform_->hWnd)
        return;

    RECT bounds{};
    if (!GetClientRect(platform_->hWnd, &bounds))
        return;

    HRESULT hr = platform_->webViewController->put_Bounds(bounds);
    assert(SUCCEEDED(hr));
}

void Photino::FocusWebView2() const
{
    if (platform_->webViewController)
    {
        platform_->webViewController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
    }
}

void Photino::NotifyWebView2WindowMove() const
{
    if (platform_->webViewController)
    {
        // MessageBox(nullptr, L"NotifyWebView2WindowMove() was called!", L"", MB_OK);
        platform_->webViewController->NotifyParentWindowPositionChanged();
    }
}

void Photino::CloseWebView()
{
    if (platform_->webViewWindow != nullptr)
    {
        platform_->webViewWindow->Stop();
        platform_->webViewWindow = nullptr;
    }

    if (platform_->webViewController != nullptr)
    {
        platform_->webViewController->Close();
        platform_->webViewController = nullptr;
    }

    if (platform_->webViewEnvironment != nullptr)
    {
        platform_->webViewEnvironment = nullptr;
    }

    platform_->webViewInitialized = false;
    platform_->scriptId.clear();
}

bool Photino::RegisterCustomSchemeName(const PlatformString& scheme)
{
    return true;
}

void Photino::SetWebView2RuntimePath(const PlatformString& pathToWebView2)
{
    g_webview2RuntimePath = pathToWebView2;
}

bool Photino::EnsureWebViewIsInstalled()
{
    LPWSTR versionInfo = nullptr;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &versionInfo);

    if (versionInfo)
        CoTaskMemFree(versionInfo);

    if (FAILED(hr))
        return InstallWebView2();

    return true;
}

bool Photino::InstallWebView2()
{
    const wchar_t* srcUrl = L"https://go.microsoft.com/fwlink/p/?LinkId=2124703";

    wchar_t tempPath[MAX_PATH];
    if (!GetTempPathW(_countof(tempPath), tempPath))
        return false;

    wchar_t destFile[MAX_PATH];
    if (wcscpy_s(destFile, tempPath) != 0)
        return false;

    if (wcscat_s(destFile, L"MicrosoftEdgeWebview2Setup.exe") != 0)
        return false;

    if (URLDownloadToFileW(nullptr, srcUrl, destFile, 0, nullptr) != S_OK)
        return false;

    wchar_t command[MAX_PATH + 3];
    if (swprintf_s(command, L"\"%s\"", destFile) < 0)
    {
        DeleteFileW(destFile);
        return false;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    BOOL success = CreateProcessW(
        nullptr, // No module name (use command line)
        command, // Command line
        nullptr, // Process handle not inheritable
        nullptr, // Thread handle not inheritable
        FALSE,   // Set handle inheritance to FALSE
        0,       // No creation flags
        nullptr, // Use parent's environment block
        nullptr, // Use parent's starting directory
        &si,     // Pointer to STARTUPINFO structure
        &pi);    // Pointer to PROCESS_INFORMATION structure

    if (!success)
    {
        DeleteFileW(destFile);
        return false;
    }

    DWORD waitResult = WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    BOOL gotExitCode = waitResult == WAIT_OBJECT_0 &&
                       GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    DeleteFileW(destFile);

    return gotExitCode && exitCode == 0;
}