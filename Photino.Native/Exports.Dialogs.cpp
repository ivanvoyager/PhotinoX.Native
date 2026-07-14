#include "Photino.Dialog.h"
#include "Photino.Export.h"
#include "Photino.h"
#include "Photino.Strings.h"

#include <cassert>

using namespace PhotinoX::Native;

extern "C"
{
    PHOTINO_EXPORT char** Photino_ShowOpenFile(const Photino* instance, Utf8String title, Utf8String defaultPath, bool multiSelect, Utf8String* filters, int filterCount, int* resultCount)
    {
        assert(instance && resultCount);
        if (!instance || !resultCount)
            return nullptr;

        *resultCount = 0;

        const auto dialog = instance->GetDialog();
        assert(dialog);
        if (!dialog)
            return nullptr;

        auto platformFilters = ToPlatformStringList(filters, filterCount);

        auto result = dialog->ShowOpenFile(ToPlatformString(title), ToPlatformString(defaultPath), multiSelect, platformFilters);

        return CopyUtf8StringArray(result, resultCount);
    }

    PHOTINO_EXPORT char** Photino_ShowOpenFolder(const Photino* instance, Utf8String title, Utf8String defaultPath, bool multiSelect, int* resultCount)
    {
        assert(instance && resultCount);
        if (!instance || !resultCount)
            return nullptr;

        *resultCount = 0;

        const auto dialog = instance->GetDialog();
        assert(dialog);
        if (!dialog)
            return nullptr;

        auto result = dialog->ShowOpenFolder(ToPlatformString(title), ToPlatformString(defaultPath), multiSelect);

        return CopyUtf8StringArray(result, resultCount);
    }

    PHOTINO_EXPORT char* Photino_ShowSaveFile(const Photino* instance, Utf8String title, Utf8String defaultPath, Utf8String* filters, int filterCount, Utf8String defaultFileName)
    {
        assert(instance);
        if (!instance)
            return nullptr;

        const auto dialog = instance->GetDialog();
        assert(dialog);
        if (!dialog)
            return nullptr;

        auto platformFilters = ToPlatformStringList(filters, filterCount);

        auto result = dialog->ShowSaveFile(ToPlatformString(title), ToPlatformString(defaultPath), platformFilters, ToPlatformString(defaultFileName));

        if (result.empty())
            return nullptr;

        return CopyUtf8String(ToUtf8String(result));
    }

    PHOTINO_EXPORT DialogResult Photino_ShowMessage(const Photino* instance, Utf8String title, Utf8String text, DialogButtons buttons, DialogIcon icon)
    {
        assert(instance);
        if (!instance)
            return DialogResult::Cancel;

        const auto dialog = instance->GetDialog();
        assert(dialog);
        if (!dialog)
            return DialogResult::Cancel;

        return dialog->ShowMessage(ToPlatformString(title), ToPlatformString(text), buttons, icon);
    }
}