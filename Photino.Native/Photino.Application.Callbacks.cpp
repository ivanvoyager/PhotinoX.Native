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

void PhotinoApplication::InvokeNotificationActivated(int notificationId, void* state) const
{
    if (notificationActivatedCallback_) notificationActivatedCallback_(notificationId, state);
}

void PhotinoApplication::InvokeNotificationActionActivated(int notificationId, int actionIndex, void* state) const
{
    if (notificationActionActivatedCallback_) notificationActionActivatedCallback_(notificationId, actionIndex, state);
}

void PhotinoApplication::InvokeNotificationInputActivated(int notificationId, Utf8String response, void* state) const
{
    if (notificationInputActivatedCallback_) notificationInputActivatedCallback_(notificationId, response, state);
}

void PhotinoApplication::InvokeNotificationDismissed(int notificationId, PhotinoNotificationDismissalReason reason, void* state) const
{
    if (notificationDismissedCallback_) notificationDismissedCallback_(notificationId, reason, state);
}

void PhotinoApplication::InvokeNotificationFailed(int notificationId, void* state) const
{
    if (notificationFailedCallback_) notificationFailedCallback_(notificationId, state);
}