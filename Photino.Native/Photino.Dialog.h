#pragma once

#include "Photino.Strings.h"

#include <vector>

#ifdef __APPLE__
#include <Cocoa/Cocoa.h>
#endif

namespace PhotinoX::Native
{
    class Photino;

    enum class DialogResult : int
    {
        Cancel = -1,
        Ok,
        Yes,
        No,
        Abort,
        Retry,
        Ignore,
    };

    enum class DialogButtons : int
    {
        Ok,
        OkCancel,
        YesNo,
        YesNoCancel,
        RetryCancel,
        AbortRetryIgnore,
    };

    enum class DialogIcon : int
    {
        Info,
        Warning,
        Error,
        Question,
    };

    class PhotinoDialog
    {
    public:
#ifdef _WIN32
        explicit PhotinoDialog(Photino* window);
#else
        PhotinoDialog();
#endif
        ~PhotinoDialog();

        std::vector<PlatformString> ShowOpenFile(const PlatformString& title, const PlatformString& defaultPath, bool multiSelect, const std::vector<PlatformString>& filters) const;
        std::vector<PlatformString> ShowOpenFolder(const PlatformString& title, const PlatformString& defaultPath, bool multiSelect) const;
        PlatformString ShowSaveFile(const PlatformString& title, const PlatformString& defaultPath, const std::vector<PlatformString>& filters, const PlatformString& defaultFileName) const;
        DialogResult ShowMessage(const PlatformString& title, const PlatformString& text, DialogButtons buttons, DialogIcon icon) const;

    protected:
#ifdef __APPLE__
        NSImage* _errorIcon;
        NSImage* _infoIcon;
        NSImage* _questionIcon;
        NSImage* _warningIcon;
#elif _WIN32
        Photino* _window = nullptr;
        bool _comInitialized = false;
#endif
    };

} // namespace PhotinoX::Native