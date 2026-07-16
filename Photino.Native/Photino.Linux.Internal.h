#pragma once

#ifdef __linux__

#include <memory>

#include <glib-object.h>
#include <webkit2/webkit2.h>

struct GObjectDeleter
{
    void operator()(gpointer object) const noexcept
    {
        if (object)
            g_object_unref(object);
    }
};

template <typename T>
using GObjectPtr = std::unique_ptr<T, GObjectDeleter>;

struct WebKitUserScriptDeleter
{
    void operator()(WebKitUserScript* script) const noexcept
    {
        if (script)
            webkit_user_script_unref(script);
    }
};

using WebKitUserScriptPtr = std::unique_ptr<WebKitUserScript, WebKitUserScriptDeleter>;

#endif