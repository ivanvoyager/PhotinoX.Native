#pragma once

#include "Photino.Enums.h"
#include "Photino.Geometry.h"
#include "Photino.Strings.h"

namespace PhotinoX::Native
{
    using VoidStateCallback = void (*)(void* state);
    using InvokeStateCallback = VoidStateCallback;

    // window-level callbacks
    using ClosingCallback = bool (*)(void* state);
    using ClosedCallback = VoidStateCallback;
    using FocusInCallback = VoidStateCallback;
    using FocusOutCallback = VoidStateCallback;
    using ResizedCallback = void (*)(int width, int height, void* state);
    using MovedCallback = void (*)(int x, int y, void* state);
    using MaximizedCallback = VoidStateCallback;
    using RestoredCallback = VoidStateCallback;
    using MinimizedCallback = VoidStateCallback;
    using FullScreenChangedCallback = void (*)(bool fullScreen, void* state);
    using StateChangedCallback = void (*)(PhotinoWindowState oldState, PhotinoWindowState newState, void* state);
    using WebMessageReceivedCallback = void (*)(Utf8String message, Utf8String uri, void* state);
    using CustomSchemeCallback = void* (*)(Utf8String url, int* numBytes, Utf8String* contentType, void* state);
    using NavigationStartingCallback = bool (*)(Utf8String uri, void* state); // returns true to cancel
    using NewWindowRequestedCallback = bool (*)(Utf8String uri, void* state); // returns true when handled
    using ContentLoadingCallback = void (*)(Utf8String uri, void* state);
    using ContentLoadedCallback = void (*)(Utf8String uri, void* state);

    using GetAllMonitorsCallback = bool (*)(const Monitor*, void* state);

    // application-level callbacks
    using StartupCallback = VoidStateCallback;
    using ShutdownRequestedCallback = bool (*)(PhotinoShutdownRequestReason reason, void* state);
    using ExitCallback = int (*)(int exitCode, void* state);

    using NotificationActivatedCallback = void (*)(int notificationId, void* state);
    using NotificationActionActivatedCallback = void (*)(int notificationId, int actionIndex, void* state);
    using NotificationInputActivatedCallback = void (*)(int notificationId, Utf8String response, void* state);
    using NotificationDismissedCallback = void (*)(int notificationId, PhotinoNotificationDismissalReason reason, void* state);
    using NotificationFailedCallback = void (*)(int notificationId, void* state);
}