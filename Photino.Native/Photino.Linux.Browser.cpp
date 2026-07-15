#include "Photino.h"
#include "Photino.Linux.Internal.h"
#include "Photino.Callbacks.h"
#include "Photino.Memory.h"
#include "Photino.Strings.h"

#include "Dependencies/json.hpp"
#include <JavaScriptCore/JavaScript.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <cassert>

using json = nlohmann::json;
using namespace PhotinoX::Native;

struct InvokeJSWaitInfo
{
    bool isCompleted = false;
};

void Photino::GetTransparentEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _transparentEnabled;
}

void Photino::SetTransparentEnabled(bool enabled)
{
    _transparentEnabled = enabled;

    assert(_window);
    if (!_window) return;

    gtk_window_set_decorated(GTK_WINDOW(_window), !_chromeless && !enabled); // hide/show window chrome

    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(_window));
    if (!screen) return;

    GdkVisual* rgbaVisual = gdk_screen_get_rgba_visual(screen);
    if (!rgbaVisual) return;

    gtk_widget_set_visual(GTK_WIDGET(_window), rgbaVisual);
    gtk_widget_set_app_paintable(GTK_WIDGET(_window), true);

    if (!_webview) return;

    GdkRGBA color;
    webkit_web_view_get_background_color(WEBKIT_WEB_VIEW(_webview), &color);

    color.alpha = enabled ? 0 : 1;

    webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(_webview), &color);
}

void Photino::ClearBrowserAutoFill() const
{
    // TODO
}

void Photino::NavigateToString(const PlatformString& content) const
{
    assert(_webview);
    if (!_webview) return;

    webkit_web_view_load_html(WEBKIT_WEB_VIEW(_webview), content.c_str(), nullptr);
}

void Photino::NavigateToUrl(const PlatformString& url) const
{
    assert(_webview);
    if (!_webview || url.empty()) return;

    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(_webview), url.c_str());
}

static void webview_eval_finished(GObject* object, GAsyncResult* result, gpointer userdata)
{
    auto waitInfo = static_cast<InvokeJSWaitInfo*>(userdata);
    if (!waitInfo) return;

    GError* error = nullptr;
    JSCValue* value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), result, &error);

    if (error)
    {
        g_warning("Failed to dispatch message to WebView: %s", error->message);
        g_error_free(error);
    }

    if (value)
        g_object_unref(value);

    waitInfo->isCompleted = true;
}

void Photino::SendWebMessage(const PlatformString& message) const
{
    assert(_webview);
    if (!_webview)
        return;

    PlatformString js;
    js.append("__dispatchMessageCallback(");
    js.append(json(message).dump(-1, ' ', false, json::error_handler_t::replace));
    js.append(")");

    InvokeJSWaitInfo invokeJsWaitInfo{};

    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(_webview),
        js.c_str(),
        -1,
        nullptr,
        nullptr,
        nullptr,
        webview_eval_finished,
        &invokeJsWaitInfo);

    while (!invokeJsWaitInfo.isCompleted)
        g_main_context_iteration(nullptr, TRUE);
}

void Photino::GetContextMenuEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _contextMenuEnabled;
}

void Photino::SetContextMenuEnabled(bool enabled)
{
    _contextMenuEnabled = enabled;
}

void Photino::GetZoomEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _zoomEnabled;
}

void Photino::SetZoomEnabled(bool enabled)
{
    _zoomEnabled = enabled;
    //! Not implemented (supported?) on Linux
}

void Photino::GetZoom(int* zoom) const
{
    assert(zoom);
    if (!zoom) return;

    *zoom = _zoom;

    if (!_webview) return;

    double rawValue = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(_webview));
    rawValue = (rawValue * 100.0) + 0.5;
    *zoom = static_cast<int>(rawValue);
}

void Photino::SetZoom(int zoom)
{
    if (zoom < 25)
        zoom = 25;
    else if (zoom > 500)
        zoom = 500;
    _zoom = zoom;

    if (!_webview) return;

    double newZoom = static_cast<double>(zoom) / 100.0;
    webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(_webview), newZoom);
}

void Photino::GetDevToolsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _devToolsEnabled;

    if (!_webview) return;

    WebKitSettings* settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(_webview));
    if (!settings)
        return;

    *enabled = webkit_settings_get_enable_developer_extras(settings) ? true : false;
}

void Photino::SetDevToolsEnabled(bool enabled)
{
    _devToolsEnabled = enabled;

    if (!_webview) return;

    WebKitSettings* settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(_webview));
    if (!settings) return;

    webkit_settings_set_enable_developer_extras(settings, enabled);
}

void Photino::GetGrantBrowserPermissions(bool* grant) const
{
    assert(grant);
    if (!grant) return;

    *grant = _grantBrowserPermissions;
}

void Photino::GetMediaAutoplayEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _mediaAutoplayEnabled;
}

void Photino::GetFileSystemAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _fileSystemAccessEnabled;
}

void Photino::GetWebSecurityEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _webSecurityEnabled;
}

void Photino::GetJavascriptClipboardAccessEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _javascriptClipboardAccessEnabled;
}

void Photino::GetMediaStreamEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _mediaStreamEnabled;
}

void Photino::GetSmoothScrollingEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _smoothScrollingEnabled;
}

void Photino::GetIgnoreCertificateErrorsEnabled(bool* enabled) const
{
    assert(enabled);
    if (!enabled) return;

    *enabled = _ignoreCertificateErrorsEnabled;
}

void Photino::SetWebKitSettings()
{
    assert(_webview);
    if (!_webview) return;

    // Rely on webkit_settings_new_with_settings to set the default settings
    // instead of using the webkit2gtk API to set the properties.
    // https://webkitgtk.org/reference/webkitgtk/stable/ctor.Settings.new_with_settings.html
    GObjectPtr<WebKitSettings> settings(webkit_settings_new_with_settings(
        // Set Photino-specific default settings
        "allow_modal_dialogs", TRUE,                       // default: FALSE
        "allow_top_navigation_to_data_urls", TRUE,         // default: FALSE
        "allow_universal_access_from_file_urls", TRUE,     // default: FALSE
        "enable_back_forward_navigation_gestures", TRUE,   // default: FALSE
        "enable_media_capabilities", TRUE,                 // default: FALSE
        "enable_mock_capture_devices", TRUE,               // default: FALSE
        "enable_page_cache", TRUE,                         // default: FALSE
        "enable_webrtc", TRUE,                             // default: FALSE
        "javascript_can_open_windows_automatically", TRUE, // default: FALSE

        // Set user-defined settings
        "allow_file_access_from_file_urls", _fileSystemAccessEnabled,         // default: FALSE
        "disable_web_security", !_webSecurityEnabled,                         // default: FALSE
        "enable_developer_extras", _devToolsEnabled,                          // default: FALSE
        "enable_media_stream", _mediaStreamEnabled,                           // default: FALSE
        "enable_smooth_scrolling", _smoothScrollingEnabled,                   // default: TRUE
        "javascript_can_access_clipboard", _javascriptClipboardAccessEnabled, // default: FALSE
        "media_playback_requires_user_gesture", !_mediaAutoplayEnabled,       // default: FALSE
        "user_agent", !_userAgent.empty() ? _userAgent.c_str() : nullptr,     // default: None

        // Other available settings for reference
        // "default_charset", "iso-8859-1",										// default: iso-8859-1
        // "cursive_font_family", "serif",										// default: serif
        // "default_font_family", "sans-serif",									// default: sans-serif
        // "fantasy_font_family", "serif",										// default: serif
        // "monospace_font_family", "monospace",								// default: monospace
        // "pictograph_font_family", "serif",									// default: serif
        // "sans_serif_font_family", "sans-serif",								// default: sans-serif
        // "minimum_font_size", 0,												// default: 0
        // "default_font_size", 16,												// default: 16
        // "default_monospace_font_size", 13,									// default: 13
        // "auto_load_images", TRUE,											// default: TRUE
        // "enable_fullscreen", TRUE,											// default: TRUE
        // "enable_html5_database", TRUE,										// default: TRUE
        // "enable_html5_local_storage", TRUE,									// default: TRUE
        // "enable_hyperlink_auditing", TRUE,									// default: TRUE
        // "enable_javascript", TRUE,											// default: TRUE
        // "enable_javascript_markup", TRUE,									// default: TRUE
        // "enable_media", TRUE,												// default: TRUE
        // "enable_mediasource", TRUE,											// default: TRUE
        // "enable_offline_web_application_cache", TRUE,						// default: TRUE
        // "enable_resizable_text_areas", TRUE,									// default: TRUE
        // "enable_site_specific_quirks", TRUE,									// default: TRUE
        // "enable_tabs_to_links", TRUE,										// default: TRUE
        // "enable_webaudio", TRUE,												// default: TRUE
        // "enable_webgl", TRUE,												// default: TRUE
        // "enable_xss_auditor", TRUE,											// default: TRUE
        // "media_playback_allows_inline", TRUE,								// default: TRUE
        // "print_backgrounds", TRUE,											// default: TRUE
        // "draw_compositing_indicators", FALSE,								// default: FALSE
        // "enable_accelerated_2d_canvas", FALSE,								// default: FALSE
        // "enable_caret_browsing", FALSE,										// default: FALSE
        // "enable_dns_prefetching", FALSE,										// default: FALSE
        // "enable_encrypted_media", FALSE,										// default: FALSE
        // "enable_frame_flattening", FALSE,									// default: FALSE
        // "enable_java", FALSE,												// default: FALSE
        // "enable_plugins", FALSE,												// default: FALSE
        // "enable_private_browsing", FALSE,									// default: FALSE
        // "enable_spatial_navigation", FALSE,									// default: FALSE
        // "enable_write_console_messages_to_stdout", FALSE,					// default: FALSE
        // "load_icons_ignoring_image_load_setting", FALSE,						// default: FALSE
        // "zoom_text_only", FALSE, 											// default: FALSE
        // "media_content_types_requiring_hardware_support", None,				// default: None
        // "hardware_acceleration_policy", WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS,	// default: WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
        nullptr)); // NULL terminates the list
    if (!settings)
        std::abort();

    if (!_browserControlInitParameters.empty())
        SetWebKitCustomSettings(settings.get()); // if any custom init parameters were passed, set them now.

    WebKitWebsiteDataManager* manager = webkit_web_view_get_website_data_manager(WEBKIT_WEB_VIEW(_webview));
    if (manager)
    {
        webkit_website_data_manager_set_tls_errors_policy(manager,
                                                          _ignoreCertificateErrorsEnabled ? WEBKIT_TLS_ERRORS_POLICY_IGNORE : WEBKIT_TLS_ERRORS_POLICY_FAIL);
    }

    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(_webview), settings.get()); // apply the settings to the webview
}

void Photino::SetWebKitCustomSettings(WebKitSettings* settings)
{
    assert(settings);
    if (!settings) return;

    // parse the JSON out of _browserControlInitParameters
    json data = json::parse(_browserControlInitParameters, nullptr, false);
    if (data.is_discarded() || !data.is_object())
    {
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Invalid WebKit custom settings JSON.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        std::abort();
    }

    for (auto it = data.begin(); it != data.end(); ++it)
    {
        // Use g_object_set_property to set the property on the settings object
        // instead of relying on the webkit2gtk API to set the properties.
        // https://docs.gtk.org/gobject/method.Object.set_property.html
        std::string propertyName = it.key();
        const json& value = it.value();

        GValue propertyValue = G_VALUE_INIT;

        if (value.is_string())
        {
            g_value_init(&propertyValue, G_TYPE_STRING);
            g_value_set_string(&propertyValue, value.get<std::string>().c_str());
        }
        else if (value.is_boolean())
        {
            g_value_init(&propertyValue, G_TYPE_BOOLEAN);
            g_value_set_boolean(&propertyValue, value.get<bool>());
        }
        else if (value.is_number_integer())
        {
            g_value_init(&propertyValue, G_TYPE_INT);
            g_value_set_int(&propertyValue, value.get<int>());
        }
        else if (value.is_number_float())
        {
            g_value_init(&propertyValue, G_TYPE_DOUBLE);
            g_value_set_double(&propertyValue, value.get<double>());
        }
        else
        {
            // Throw an error
            GtkWidget* dialog = gtk_message_dialog_new(
                nullptr, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Invalid value type for key: %s", propertyName.c_str());
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            std::abort();
        }

        g_object_set_property(G_OBJECT(settings), propertyName.c_str(), &propertyValue);
        g_value_unset(&propertyValue);
    }
}

static void FreeNativeMemory(gpointer data)
{
    FreeMemory(data);
}

static void FinishCustomSchemeRequestWithError(WebKitURISchemeRequest* request, const char* message)
{
    GError* error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_FAILED, message);
    webkit_uri_scheme_request_finish_error(request, error);
    g_error_free(error);
}

void HandleCustomSchemeRequest(WebKitURISchemeRequest* request, gpointer userData)
{
    assert(request);
    if (!request) return;

    auto callback = reinterpret_cast<WebResourceRequestedCallback>(userData);
    if (!callback)
    {
        FinishCustomSchemeRequestWithError(request, "Custom scheme callback is not registered.");
        return;
    }

    const gchar* uri = webkit_uri_scheme_request_get_uri(request);
    if (!uri || *uri == '\0')
    {
        FinishCustomSchemeRequestWithError(request, "Custom scheme request URI is empty.");
        return;
    }

    int numBytes = 0;
    Utf8String contentType = nullptr;
    void* responseData = callback(uri, &numBytes, &contentType);

    if (!responseData || numBytes <= 0)
    {
        FreeMemory(responseData);
        FreeString(const_cast<char*>(contentType));

        FinishCustomSchemeRequestWithError(request, "Custom scheme response is empty.");
        return;
    }

    GObjectPtr<GInputStream> stream(g_memory_input_stream_new_from_data(responseData, numBytes, FreeNativeMemory));
    if (!stream)
    {
        FreeMemory(responseData);
        FreeString(const_cast<char*>(contentType));

        FinishCustomSchemeRequestWithError(request, "Failed to create custom scheme response stream.");
        return;
    }

    webkit_uri_scheme_request_finish(
        request,
        stream.get(),
        numBytes,
        contentType);

    FreeString(const_cast<char*>(contentType));
}

void Photino::AddCustomSchemeHandlers()
{
    assert(_webview);
    if (!_webview || !_customSchemeCallback) return;

    WebKitWebContext* context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(_webview));
    if (!context) return;

    for (const auto& scheme : _customSchemeNames)
    {
        webkit_web_context_register_uri_scheme(
            context,
            scheme.c_str(),
            HandleCustomSchemeRequest,
            reinterpret_cast<void*>(_customSchemeCallback),
            nullptr);
    }
}

bool Photino::RegisterCustomSchemeName(const PlatformString& scheme)
{
    if (!_webview) return true;

    if (!_customSchemeCallback) return false;

    WebKitWebContext* context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(_webview));
    if (!context) return false;

    if (scheme.empty()) return false;

    webkit_web_context_register_uri_scheme(
        context,
        scheme.c_str(),
        HandleCustomSchemeRequest,
        reinterpret_cast<void*>(_customSchemeCallback),
        nullptr);

    return true;
}