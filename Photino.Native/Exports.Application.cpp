#include "Photino.Application.h"
#include "Photino.Export.h"

using namespace PhotinoX::Native;

extern "C"
{
    PHOTINO_EXPORT int PhotinoApplication_Run()
    {
        try
        {
            return PhotinoApplication::Instance().Run();
        }
        catch (...)
        {
            return -1;
        }
    }

    PHOTINO_EXPORT void PhotinoApplication_Shutdown(const int exitCode)
    {
        PhotinoApplication::Instance().Shutdown(exitCode);
    }

    PHOTINO_EXPORT bool PhotinoApplication_IsRunning()
    {
        return PhotinoApplication::Instance().IsRunning();
    }

    PHOTINO_EXPORT bool PhotinoApplication_IsShuttingDown()
    {
        return PhotinoApplication::Instance().IsShuttingDown();
    }

    PHOTINO_EXPORT bool PhotinoApplication_CheckAccess()
    {
        return PhotinoApplication::Instance().CheckAccess();
    }

    PHOTINO_EXPORT bool PhotinoApplication_Invoke(const InvokeStateCallback callback, void* state)
    {
        return PhotinoApplication::Instance().Invoke(callback, state);
    }

    PHOTINO_EXPORT bool PhotinoApplication_BeginInvoke(const InvokeStateCallback callback, void* state)
    {
        return PhotinoApplication::Instance().BeginInvoke(callback, state);
    }
}