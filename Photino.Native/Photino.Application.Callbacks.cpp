#include "Photino.Application.h"

using namespace PhotinoX::Native;

void PhotinoApplication::InvokeStartup() const
{
    if (startupCallback_) startupCallback_(callbackState_);
}

bool PhotinoApplication::InvokeShutdownRequested(PhotinoShutdownRequestReason reason) const
{
    if (!shutdownRequestedCallback_ || isShutdownRequested_)
        return false;

    isShutdownRequested_ = true;
    bool result = shutdownRequestedCallback_(reason, callbackState_);
    isShutdownRequested_ = false;

    return result; // ShutdownRequested: true = cancel shutdown, false = allow shutdown
}

int PhotinoApplication::InvokeExit(int exitCode) const
{
    if (exitCallback_)
    {
        exitCode = exitCallback_(exitCode, callbackState_);
    }
    return exitCode;
}

bool PhotinoApplication::InvokeWindowCollectionChanged(NotifyCollectionChangedAction action,
    void* const* newItems, int newItemsCount, void* const* oldItems, int oldItemsCount) const
{
    if (windowCollectionChangedCallback_)
    {
        windowCollectionChangedCallback_(action, newItems, newItemsCount, oldItems, oldItemsCount, callbackState_);
        return true;
    }
    return false;
}

void PhotinoApplication::InvokeNotificationActivated(int notificationId, void* state) const
{
    if (notificationActivatedCallback_) notificationActivatedCallback_(notificationId, state, callbackState_);
}

void PhotinoApplication::InvokeNotificationActionActivated(int notificationId, int actionIndex, void* state) const
{
    if (notificationActionActivatedCallback_) notificationActionActivatedCallback_(notificationId, actionIndex, state, callbackState_);
}

void PhotinoApplication::InvokeNotificationInputActivated(int notificationId, Utf8String response, void* state) const
{
    if (notificationInputActivatedCallback_) notificationInputActivatedCallback_(notificationId, response, state, callbackState_);
}

void PhotinoApplication::InvokeNotificationDismissed(int notificationId, PhotinoNotificationDismissalReason reason, void* state) const
{
    if (notificationDismissedCallback_) notificationDismissedCallback_(notificationId, reason, state, callbackState_);
}

void PhotinoApplication::InvokeNotificationFailed(int notificationId, void* state) const
{
    if (notificationFailedCallback_) notificationFailedCallback_(notificationId, state, callbackState_);
}