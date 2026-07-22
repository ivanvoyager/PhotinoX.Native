#include "Photino.Application.h"

#include <atomic>
#include <cassert>

#include <Windows.h>

using namespace PhotinoX::Native;

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

void PhotinoApplication::ShutdownCore(int exitCode) noexcept
{
    HWND messageWindow = g_messageWindow.load(std::memory_order_acquire);
    if (messageWindow && IsWindow(messageWindow))
        PostMessageW(messageWindow, WM_PHOTINO_SHUTDOWN, static_cast<WPARAM>(exitCode), 0);
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