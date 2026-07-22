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
    using ResourceCallback = void* (*)(Utf8String, int*, Utf8String*);
    using MonitorCallback = int (*)(const Monitor*);

    using InvokeCallback = VoidCallback;
    using InvokeStateCallback = VoidStateCallback;

    //no parameters, no return value
    using MaximizedCallback = VoidCallback;
    using RestoredCallback = VoidCallback;
    using MinimizedCallback = VoidCallback;
    using FocusInCallback = VoidCallback;
    using FocusOutCallback = VoidCallback;
    using ClosedCallback = VoidCallback;

    //with parameters and/or return value
    using ClosingCallback = BoolCallback;
    using ResizedCallback = IntIntCallback; //(int width, int height)
    using MovedCallback = IntIntCallback;   //(int x, int y)
    using WebMessageReceivedCallback = StringCallback;
    using WebResourceRequestedCallback = ResourceCallback;
    using GetAllMonitorsCallback = MonitorCallback;
    using FullScreenChangedCallback = VoidBoolCallback;
    using StateChangedCallback = void(*)(WindowState oldState, WindowState newState);
}