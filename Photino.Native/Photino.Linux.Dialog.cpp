#ifdef __linux__
#include "Photino.Dialog.h"

#include <gtk/gtk.h>

using namespace PhotinoX::Native;

namespace
{
    enum class DialogType
    {
        OpenFile,
        OpenFolder,
        SaveFile
    };

    void AddFilters(GtkWidget* dialog, const std::vector<PlatformString>& filters)
    {
        if (!dialog || filters.empty())
            return;

        for (const auto& filterValue : filters)
        {
            const auto separator = filterValue.find('|');
            if (separator == PlatformString::npos || separator == 0 || separator + 1 >= filterValue.size())
                continue;

            PlatformString name = filterValue.substr(0, separator);
            PlatformString patterns = filterValue.substr(separator + 1);

            GtkFileFilter* filter = gtk_file_filter_new();
            gtk_file_filter_set_name(filter, name.c_str());

            bool hasPattern = false;
            size_t start = 0;

            while (start < patterns.size())
            {
                const auto end = patterns.find(';', start);
                PlatformString pattern = patterns.substr(
                    start,
                    end == PlatformString::npos ? PlatformString::npos : end - start);

                if (!pattern.empty())
                {
                    gtk_file_filter_add_pattern(filter, pattern.c_str());
                    hasPattern = true;
                }

                if (end == PlatformString::npos)
                    break;

                start = end + 1;
            }

            if (hasPattern)
                gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
            else
                g_object_unref(filter);
        }
    }

    std::vector<PlatformString> ShowDialog(
        DialogType type,
        const PlatformString& title,
        const PlatformString& defaultPath,
        bool multiSelect,
        const std::vector<PlatformString>& filters,
        const PlatformString& defaultFileName = {})
    {
        GtkFileChooserAction action;
        const char* buttonText = nullptr;

        switch (type)
        {
        case DialogType::OpenFile:
            action = GTK_FILE_CHOOSER_ACTION_OPEN;
            buttonText = "_Open";
            break;
        case DialogType::OpenFolder:
            action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
            buttonText = "_Select";
            break;
        case DialogType::SaveFile:
            action = GTK_FILE_CHOOSER_ACTION_SAVE;
            buttonText = "_Save";
            break;
        default:
            return {};
        }

        GtkWidget* dialog = gtk_file_chooser_dialog_new(
            title.c_str(),
            nullptr,//GTK_WINDOW(parent_window)
            action,
            "_Cancel",
            GTK_RESPONSE_CANCEL,
            buttonText,
            GTK_RESPONSE_ACCEPT,
            nullptr);

        if (!dialog)
            return {};

        //gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent_window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

        if (!defaultPath.empty())
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), defaultPath.c_str());

        if (type == DialogType::OpenFile || type == DialogType::OpenFolder)
            gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), multiSelect ? TRUE : FALSE);

        if (type == DialogType::SaveFile)
        {
            gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

            if (!defaultFileName.empty())
                gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), defaultFileName.c_str());
        }

        if (type == DialogType::OpenFile || type == DialogType::SaveFile)
            AddFilters(dialog, filters);

        std::vector<PlatformString> result;

        const gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == GTK_RESPONSE_ACCEPT)
        {
            if (type == DialogType::OpenFile || type == DialogType::OpenFolder)
            {
                GSList* pathList = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));

                for (GSList* item = pathList; item; item = item->next)
                {
                    auto* path = static_cast<char*>(item->data);
                    if (path)
                    {
                        result.emplace_back(path);
                        g_free(path);
                    }
                }

                g_slist_free(pathList);
            }
            else
            {
                char* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
                if (path)
                {
                    result.emplace_back(path);
                    g_free(path);
                }
            }
        }

        gtk_widget_destroy(dialog);
        return result;
    }
}

PhotinoDialog::PhotinoDialog()
{
}

PhotinoDialog::~PhotinoDialog()
{
}

std::vector<PlatformString> PhotinoDialog::ShowOpenFile(
    const PlatformString& title,
    const PlatformString& defaultPath,
    bool multiSelect,
    const std::vector<PlatformString>& filters) const
{
    return ShowDialog(DialogType::OpenFile, title, defaultPath, multiSelect, filters);
}

std::vector<PlatformString> PhotinoDialog::ShowOpenFolder(
    const PlatformString& title,
    const PlatformString& defaultPath,
    bool multiSelect) const
{
    return ShowDialog(DialogType::OpenFolder, title, defaultPath, multiSelect, {});
}

PlatformString PhotinoDialog::ShowSaveFile(
    const PlatformString& title,
    const PlatformString& defaultPath,
    const std::vector<PlatformString>& filters,
    const PlatformString& defaultFileName) const
{
    auto result = ShowDialog(DialogType::SaveFile, title, defaultPath, false, filters, defaultFileName);
    return result.empty() ? PlatformString() : result.front();
}

DialogResult PhotinoDialog::ShowMessage(
    const PlatformString& title,
    const PlatformString& text,
    DialogButtons buttons,
    DialogIcon icon) const
{
    GtkMessageType type;

    switch (icon)
    {
    case DialogIcon::Info:
        type = GTK_MESSAGE_INFO;
        break;
    case DialogIcon::Warning:
        type = GTK_MESSAGE_WARNING;
        break;
    case DialogIcon::Error:
        type = GTK_MESSAGE_ERROR;
        break;
    case DialogIcon::Question:
        type = GTK_MESSAGE_QUESTION;
        break;
    default:
        type = GTK_MESSAGE_OTHER;
        break;
    }

    GtkWidget* dialog = gtk_message_dialog_new(
        nullptr,
        GTK_DIALOG_MODAL,
        type,
        GTK_BUTTONS_NONE,
        "%s",
        text.c_str());

    if (!dialog)
        return DialogResult::Cancel;

    if (!title.empty())
        gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());

    switch (buttons)
    {
    case DialogButtons::Ok:
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Ok", static_cast<gint>(DialogResult::Ok));
        break;
    case DialogButtons::OkCancel:
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Ok", static_cast<gint>(DialogResult::Ok));
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Cancel", static_cast<gint>(DialogResult::Cancel));
        break;
    case DialogButtons::YesNo:
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Yes", static_cast<gint>(DialogResult::Yes));
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_No", static_cast<gint>(DialogResult::No));
        break;
    case DialogButtons::YesNoCancel:
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Yes", static_cast<gint>(DialogResult::Yes));
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_No", static_cast<gint>(DialogResult::No));
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Cancel", static_cast<gint>(DialogResult::Cancel));
        break;
    case DialogButtons::RetryCancel:
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Retry", static_cast<gint>(DialogResult::Retry));
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Cancel", static_cast<gint>(DialogResult::Cancel));
        break;
    case DialogButtons::AbortRetryIgnore:
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Abort", static_cast<gint>(DialogResult::Abort));
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Retry", static_cast<gint>(DialogResult::Retry));
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Ignore", static_cast<gint>(DialogResult::Ignore));
        break;
    default:
        gtk_dialog_add_button(GTK_DIALOG(dialog), "_Ok", static_cast<gint>(DialogResult::Ok));
        break;
    }

    const gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    switch (response)
    {
    case static_cast<gint>(DialogResult::Ok):
        return DialogResult::Ok;
    case static_cast<gint>(DialogResult::Yes):
        return DialogResult::Yes;
    case static_cast<gint>(DialogResult::No):
        return DialogResult::No;
    case static_cast<gint>(DialogResult::Cancel):
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_DELETE_EVENT:
        return DialogResult::Cancel;
    case static_cast<gint>(DialogResult::Abort):
        return DialogResult::Abort;
    case static_cast<gint>(DialogResult::Retry):
        return DialogResult::Retry;
    case static_cast<gint>(DialogResult::Ignore):
        return DialogResult::Ignore;
    default:
        return DialogResult::Cancel;
    }
}
#endif