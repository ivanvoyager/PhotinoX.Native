#include "Photino.Application.h"

#include <cassert>
#include <condition_variable>
#include <mutex>

#include <glib.h>
#include <gtk/gtk.h>

using namespace PhotinoX::Native;

namespace
{
    struct InvokeWaitInfo
    {
        InvokeCallback callback = nullptr;
        std::condition_variable completionNotifier;
        std::mutex mutex;
        bool isCompleted = false;
    };

    gboolean InvokeCallbackSync(gpointer data)
    {
        auto waitInfo = static_cast<InvokeWaitInfo*>(data);
        if (!waitInfo)
            return G_SOURCE_REMOVE;

        if (waitInfo->callback)
            waitInfo->callback();

        {
            std::lock_guard guard(waitInfo->mutex);
            waitInfo->isCompleted = true;
        }

        waitInfo->completionNotifier.notify_one();
        return G_SOURCE_REMOVE;
    }

    struct InvokeAsyncInfo
    {
        InvokeStateCallback callback = nullptr;
        void* state = nullptr;
    };

    gboolean InvokeCallbackAsync(gpointer data)
    {
        auto info = static_cast<InvokeAsyncInfo*>(data);
        if (!info)
            return G_SOURCE_REMOVE;

        if (info->callback)
            info->callback(info->state);

        return G_SOURCE_REMOVE;
    }

    void DestroyInvokeAsyncInfo(gpointer data)
    {
        delete static_cast<InvokeAsyncInfo*>(data);
    }

    gboolean ShutdownApplication(gpointer data)
    {
        gtk_main_quit();
        return G_SOURCE_REMOVE;
    }
} // namespace

int PhotinoApplication::RunCore()
{
    gtk_main();
    return exitCode_.load(std::memory_order_acquire);
}

void PhotinoApplication::ShutdownCore(int exitCode) noexcept
{
    g_main_context_invoke_full(
        g_main_context_default(),
        G_PRIORITY_DEFAULT,
        ShutdownApplication,
        nullptr,
        nullptr);
}

bool PhotinoApplication::CheckAccess() const noexcept
{
    return g_main_context_is_owner(g_main_context_default());
}

bool PhotinoApplication::Invoke(InvokeCallback callback) const
{
    assert(callback);

    if (!callback || IsShuttingDown())
        return false;

    if (CheckAccess())
    {
        callback();
        return true;
    }

    if (!IsRunning())
        return false;

    InvokeWaitInfo waitInfo{};
    waitInfo.callback = callback;

    g_main_context_invoke_full(
        g_main_context_default(),
        G_PRIORITY_DEFAULT,
        InvokeCallbackSync,
        &waitInfo,
        nullptr);

    std::unique_lock lock(waitInfo.mutex);
    waitInfo.completionNotifier.wait(lock, [&waitInfo]
                                     { return waitInfo.isCompleted; });

    return true;
}

bool PhotinoApplication::BeginInvoke(InvokeStateCallback callback, void* state) const
{
    assert(callback);

    if (!callback || IsShuttingDown() || !IsRunning())
        return false;

    auto info = new InvokeAsyncInfo{ callback, state };

    return g_idle_add_full(
               G_PRIORITY_DEFAULT,
               InvokeCallbackAsync,
               info,
               DestroyInvokeAsyncInfo) != 0;
}