#pragma once

#include "Photino.Enums.h"
#include "Photino.Monitor.h"
#include "Photino.Strings.h"

namespace PhotinoX::Native
{
    using VoidCallback = void (*)();
    using VoidStateCallback = void (*)(void* state);
    using VoidBoolCallback = void (*)(bool);
    using BoolCallback = bool (*)();
    using IntIntCallback = void (*)(int, int);  // Resized, Moved
    using StringCallback = void (*)(Utf8String);
    using StringBoolCallback = bool (*)(Utf8String);
    using StringStringCallback = void (*)(Utf8String, Utf8String);
    using ResourceCallback = void* (*)(Utf8String, int*, Utf8String*);
    using MonitorCallback = int (*)(const Monitor*, void* state);

    using InvokeStateCallback = VoidStateCallback;

    // no parameters, no return value
    using MaximizedCallback = VoidCallback;
    using RestoredCallback = VoidCallback;
    using MinimizedCallback = VoidCallback;
    using FocusInCallback = VoidCallback;
    using FocusOutCallback = VoidCallback;
    using ClosedCallback = VoidCallback;

    // with parameters and/or return value
    using ClosingCallback = BoolCallback;
    using ResizedCallback = IntIntCallback; //(int width, int height)
    using MovedCallback = IntIntCallback;   //(int x, int y)
    using WebMessageReceivedCallback = StringStringCallback; // (message, uri)
    using NavigationStartingCallback = StringBoolCallback; // returns true to cancel
    using NewWindowRequestedCallback = StringBoolCallback; // returns true when handled
    using ContentLoadingCallback = StringCallback;
    using ContentLoadedCallback = StringCallback;
    using WebResourceRequestedCallback = ResourceCallback;
    using GetAllMonitorsCallback = MonitorCallback;
    using FullScreenChangedCallback = VoidBoolCallback;
    using StateChangedCallback = void(*)(PhotinoWindowState oldState, PhotinoWindowState newState);

    // application-level callbacks
    using StartupCallback = VoidCallback;
    using ExitCallback = int (*)(int exitCode);

    using NotificationActivatedCallback = void (*)(int notificationId, void* state);
    using NotificationActionActivatedCallback = void (*)(int notificationId, int actionIndex, void* state);
    using NotificationInputActivatedCallback = void (*)(int notificationId, Utf8String response, void* state);
    using NotificationDismissedCallback = void (*)(int notificationId, PhotinoNotificationDismissalReason reason, void* state);
    using NotificationFailedCallback = void (*)(int notificationId, void* state);
}