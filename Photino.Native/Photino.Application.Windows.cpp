#include "Photino.Application.h"
#include "Photino.Application.InitParams.h"
#include "Photino.Application.Windows.State.h"
#include "Photino.Application.Windows.ToastHandler.h"
#include "Photino.Strings.h"

#include "Dependencies/wintoastlib.h"

#include <atomic>
#include <cassert>

#include <Windows.h>

using namespace PhotinoX::Native;
using namespace WinToastLib;

namespace
{
    constexpr wchar_t ApplicationWindowClassName[] = L"PhotinoXApplicationWindow";
    constexpr UINT WM_PHOTINO_INVOKE_STATE = WM_APP + 1;
    constexpr UINT WM_PHOTINO_SHUTDOWN = WM_APP + 2;

    std::atomic<DWORD> g_uiThreadId{0};
    std::atomic<HWND> g_messageWindow{nullptr};

    LRESULT CALLBACK ApplicationWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {

        case WM_QUERYENDSESSION:
        {
            auto& application = PhotinoApplication::Instance();
            if (application.IsShuttingDown())
                return TRUE;

            auto reason = (static_cast<DWORD>(lParam) & ENDSESSION_LOGOFF) != 0
                              ? PhotinoShutdownRequestReason::SessionLogoff
                              : PhotinoShutdownRequestReason::SystemShutdown;

            return application.InvokeShutdownRequested(reason) ? FALSE : TRUE;
        }

        case WM_ENDSESSION:
        {
            if (wParam == TRUE)
            {
                // SessionEnded - SystemShutdown or Logoff
                PhotinoApplication::Instance().NotifySessionEnding();
            }
            return 0;
        }

        case WM_PHOTINO_INVOKE_STATE:
        {
            auto callback = reinterpret_cast<InvokeStateCallback>(wParam);
            assert(callback);

            if (!callback)
                return FALSE;

            auto state = reinterpret_cast<void*>(lParam);
            callback(state);

            return TRUE;
        }

        case WM_PHOTINO_SHUTDOWN:
        {
            auto exitCode = static_cast<int>(wParam);
            bool force = lParam != FALSE;

            if (force)
            {
                PostQuitMessage(exitCode);
                return 0;
            }

            if (PhotinoApplication::Instance().HandleShutdownRequest(exitCode))
                PostQuitMessage(exitCode);

            return 0;
        }

        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    bool RegisterApplicationWindowClass()
    {
        WNDCLASSW wc{};
        wc.lpfnWndProc = ApplicationWindowProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = ApplicationWindowClassName;

        return RegisterClassW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }

    HWND CreateApplicationMessageWindow()
    {
        if (!RegisterApplicationWindowClass())
            return nullptr;

        return CreateWindowExW(
            0,
            ApplicationWindowClassName,
            L"PhotinoX Application Message Window",
            0,
            0,
            0,
            0,
            0,
            HWND_MESSAGE,               // Parent window handle
            nullptr,                    // Menu
            GetModuleHandleW(nullptr),  // Instance handle
            nullptr                     // Additional application data
            );
    }
} // namespace

PhotinoApplication::PhotinoApplication() : platform_(std::make_unique<WindowsApplicationState>())
{
}

PhotinoApplication::~PhotinoApplication() = default;

void PhotinoApplication::ValidateInitParams(const PhotinoApplicationInitParams* initParams)
{
    assert(initParams);
    if (!initParams)
        std::abort();

    if (initParams->Size != sizeof(PhotinoApplicationInitParams) ||
        initParams->AbiVersion != PhotinoApplicationInitParams::NativeAbiVersion)
    {
        wchar_t msg[256];

        swprintf(
            msg,
            256,
            L"Application initial parameters ABI mismatch. Passed size: %i bytes, expected size: %I64i bytes. Passed ABI version: %i, expected ABI version: %i.",
            initParams->Size,
            sizeof(PhotinoApplicationInitParams),
            initParams->AbiVersion,
            PhotinoApplicationInitParams::NativeAbiVersion);

        MessageBoxW(nullptr, msg, L"Native Initialization Failed", MB_OK);
        std::abort();
    }
}

int PhotinoApplication::RunCore()
{
    assert(!g_messageWindow.load(std::memory_order_acquire));

    g_uiThreadId.store(GetCurrentThreadId(), std::memory_order_release);

    HWND messageWindow = CreateApplicationMessageWindow();
    g_messageWindow.store(messageWindow, std::memory_order_release);

    if (!messageWindow)
    {
        g_uiThreadId.store(0, std::memory_order_release);
        return -1;
    }

    MSG message{};
    int exitCode = 0;

    // Run the message loop
    while (true)
    {
        const BOOL result = GetMessageW(&message, nullptr, 0, 0);

        if (result == -1)//error
        {
            exitCode = -1;
            break;
        }

        if (result == 0)//WM_QUIT
        {
            exitCode = static_cast<int>(message.wParam);
            break;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    messageWindow = g_messageWindow.exchange(nullptr, std::memory_order_acq_rel);
    if (messageWindow)
        DestroyWindow(messageWindow);

    g_uiThreadId.store(0, std::memory_order_release);

    return exitCode;
}

void PhotinoApplication::ShutdownCore(int exitCode, bool force) noexcept
{
    HWND messageWindow = g_messageWindow.load(std::memory_order_acquire);
    if (messageWindow && IsWindow(messageWindow))
        PostMessageW(messageWindow, WM_PHOTINO_SHUTDOWN, static_cast<WPARAM>(exitCode), force ? TRUE : FALSE);
}

bool PhotinoApplication::CheckAccess() const noexcept
{
    DWORD uiThreadId = g_uiThreadId.load(std::memory_order_acquire);
    DWORD currentThreadId = GetCurrentThreadId();
    return uiThreadId != 0 && currentThreadId == uiThreadId;
}

bool PhotinoApplication::Invoke(InvokeStateCallback callback, void* state) const
{
    assert(callback);

    if (!callback || IsShuttingDown())
        return false;

    if (CheckAccess())
    {
        callback(state);
        return true;
    }

    if (!IsRunning())
        return false;

    HWND messageWindow = g_messageWindow.load(std::memory_order_acquire);
    if (!messageWindow || !IsWindow(messageWindow))
        return false;

    return SendMessageW(
                messageWindow,
                WM_PHOTINO_INVOKE_STATE,
                reinterpret_cast<WPARAM>(callback),
                reinterpret_cast<LPARAM>(state)) == TRUE;
}

bool PhotinoApplication::BeginInvoke(InvokeStateCallback callback, void* state) const
{
    assert(callback);

    if (!callback || IsShuttingDown() || !IsRunning())
        return false;

    HWND messageWindow = g_messageWindow.load(std::memory_order_acquire);
    if (!messageWindow || !IsWindow(messageWindow))
        return false;

    return PostMessageW(
               messageWindow,
               WM_PHOTINO_INVOKE_STATE,
               reinterpret_cast<WPARAM>(callback),
               reinterpret_cast<LPARAM>(state)) != FALSE;
}

bool PhotinoApplication::InitializeNotifications()
{
    bool expected = false;
    if (!notificationsInitialized_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return true;

    if (!WinToast::isCompatible())
    {
        notificationsInitialized_.store(false, std::memory_order_release);
        return false;
    }

    auto instance = WinToast::instance();

    const auto appName = options_.applicationName.empty()
                             ? PlatformString(L"PhotinoX")
                             : options_.applicationName;

    instance->setAppName(appName);

    const auto appUserModelId = !options_.notificationRegistrationId.empty()
                                    ? options_.notificationRegistrationId
                                    : appName;

    instance->setAppUserModelId(appUserModelId);

    WinToast::WinToastError error;
    if (!instance->initialize(&error))
    {
        notificationsInitialized_.store(false, std::memory_order_release);
        return false;
    }

    return true;
}

void PhotinoApplication::UninitializeNotifications() noexcept
{
    if (!notificationsInitialized_.exchange(false, std::memory_order_acq_rel))
        return;

    WinToast::instance()->uninitialize();
}

int PhotinoApplication::ShowNotificationCore(int notificationId, const PlatformString& title, const PlatformString& body, const PlatformString& iconPath, void* callbackState)
{
    WinToastTemplate templ(!iconPath.empty() ? WinToastTemplate::ImageAndText02 : WinToastTemplate::Text02);
    templ.setTextField(title, WinToastTemplate::FirstLine);
    templ.setTextField(body, WinToastTemplate::SecondLine);

    if (!iconPath.empty())
        templ.setImagePath(iconPath);

    WinToast::WinToastError error;
    auto toastId = WinToast::instance()->showToast(templ, new WinToastHandler(this, notificationId, callbackState), &error);

    return toastId < 0 ? -3 : notificationId;
}