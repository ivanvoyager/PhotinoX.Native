#pragma once

#ifdef __APPLE__

namespace PhotinoX::Native
{
    class Photino;
} // namespace PhotinoX::Native

bool PhotinoMacIsShuttingDown();
void PhotinoMacSetShuttingDown(bool value);

void PhotinoMacStopMessageLoopIfOwner(PhotinoX::Native::Photino* owner);

#endif