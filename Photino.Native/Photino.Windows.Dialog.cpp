#include "Photino.Dialog.h"
#include "Photino.h"

#include <Windows.h>
#include <cassert>
#include <shobjidl.h>

#include <string>
#include <type_traits>
#include <vector>

using namespace PhotinoX::Native;

namespace
{
    class ActivationContextHandle
    {
      public:
        ActivationContextHandle()
            : _handle(Create())
        {
        }

        ~ActivationContextHandle()
        {
            if (_handle != INVALID_HANDLE_VALUE)
                ReleaseActCtx(_handle);
        }

        HANDLE Get() const
        {
            return _handle;
        }

      private:
        static HANDLE Create()
        {
            const UINT len = GetSystemDirectoryW(nullptr, 0);
            if (len == 0)
                return INVALID_HANDLE_VALUE;

            std::wstring sysDir(len, L'\0');
            if (GetSystemDirectoryW(sysDir.data(), len) == 0)
                return INVALID_HANDLE_VALUE;

            const ACTCTXW actCtx =
                {
                    sizeof(actCtx),
                    ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID,
                    L"shell32.dll",
                    0,
                    0,
                    sysDir.c_str(),
                    MAKEINTRESOURCEW(124),
                    nullptr,
                    nullptr,
                };

            return CreateActCtxW(&actCtx);
        }

        HANDLE _handle = INVALID_HANDLE_VALUE;
    };

    class NewStyleContext
    {
        public:
            NewStyleContext();
            ~NewStyleContext();
    
        private:
            ULONG_PTR _cookie = 0;
    };

    NewStyleContext::NewStyleContext()
    {
        static ActivationContextHandle hctx;

        if (hctx.Get() != INVALID_HANDLE_VALUE)
            ActivateActCtx(hctx.Get(), &_cookie);
    }

    NewStyleContext::~NewStyleContext()
    {
        if (_cookie != 0)
            DeactivateActCtx(0, _cookie);
    }
}

PhotinoDialog::PhotinoDialog(Photino* window)
{
    _window = window;

    const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    _comInitialized = SUCCEEDED(hr);
}

PhotinoDialog::~PhotinoDialog()
{
    if (_comInitialized)
        CoUninitialize();
}

namespace
{
    template <typename T>
    T* CreateDlg(HRESULT* hResult, const PlatformString& title, const PlatformString& defaultPath)
    {
        static_assert(std::is_base_of_v<IFileDialog, T>, "T must inherit from IFileDialog");

        assert(hResult);
        if (!hResult)
            return nullptr;

        T* pfd = nullptr;

        const CLSID clsid = std::is_same_v<T, IFileSaveDialog>
                                ? CLSID_FileSaveDialog
                                : CLSID_FileOpenDialog;

        HRESULT hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
        if (FAILED(hr))
        {
            *hResult = hr;
            return nullptr;
        }

        if (!title.empty())
        {
            hr = pfd->SetTitle(title.c_str());
            if (FAILED(hr))
            {
                pfd->Release();
                *hResult = hr;
                return nullptr;
            }
        }

        if (!defaultPath.empty())
        {
            IShellItem* psiDefault = nullptr;
            hr = SHCreateItemFromParsingName(defaultPath.c_str(), nullptr, IID_PPV_ARGS(&psiDefault));
            if (SUCCEEDED(hr) && psiDefault)
            {
                hr = pfd->SetFolder(psiDefault);
                psiDefault->Release();

                if (FAILED(hr))
                {
                    pfd->Release();
                    *hResult = hr;
                    return nullptr;
                }
            }
        }

        *hResult = S_OK;
        return pfd;
    }

    bool AddFilters(IFileDialog* pfd, const std::vector<PlatformString>& filters)
    {
        if (!pfd || filters.empty())
            return true;

        std::vector<PlatformString> names;
        std::vector<PlatformString> patterns;
        std::vector<COMDLG_FILTERSPEC> specs;

        names.reserve(filters.size());
        patterns.reserve(filters.size());
        specs.reserve(filters.size());

        for (const auto& filter : filters)
        {
            const auto separator = filter.find(L'|');
            if (separator == PlatformString::npos || separator == 0 || separator + 1 >= filter.size())
                continue;

            names.emplace_back(filter.substr(0, separator));
            patterns.emplace_back(filter.substr(separator + 1));

            COMDLG_FILTERSPEC spec{};
            spec.pszName = names.back().c_str();
            spec.pszSpec = patterns.back().c_str();
            specs.push_back(spec);
        }

        if (specs.empty())
            return true;

        return SUCCEEDED(pfd->SetFileTypes(static_cast<UINT>(specs.size()), specs.data()));
    }

    std::vector<PlatformString> GetResults(IFileOpenDialog* pfd)
    {
        std::vector<PlatformString> result;

        IShellItemArray* psiResults = nullptr;
        HRESULT hr = pfd->GetResults(&psiResults);
        if (FAILED(hr) || !psiResults)
            return result;

        DWORD count = 0;
        hr = psiResults->GetCount(&count);
        if (FAILED(hr))
        {
            psiResults->Release();
            return result;
        }

        result.reserve(static_cast<size_t>(count));

        for (DWORD i = 0; i < count; ++i)
        {
            IShellItem* psiItem = nullptr;
            hr = psiResults->GetItemAt(i, &psiItem);
            if (FAILED(hr) || !psiItem)
                continue;

            PWSTR pszName = nullptr;
            hr = psiItem->GetDisplayName(SIGDN_FILESYSPATH, &pszName);

            if (pszName)
            {
                if (SUCCEEDED(hr))
                    result.emplace_back(pszName);

                CoTaskMemFree(pszName);
            }

            psiItem->Release();
        }

        psiResults->Release();
        return result;
    }
}

std::vector<PlatformString> PhotinoDialog::ShowOpenFile(
    const PlatformString& title,
    const PlatformString& defaultPath,
    bool multiSelect,
    const std::vector<PlatformString>& filters) const
{
    assert(_window);
    if (!_window)
        return {};

    HRESULT hr = S_OK;
    IFileOpenDialog* pfd = CreateDlg<IFileOpenDialog>(&hr, title, defaultPath);
    if (!pfd)
        return {};

    if (!AddFilters(pfd, filters))
    {
        pfd->Release();
        return {};
    }

    DWORD dwOptions = 0;
    hr = pfd->GetOptions(&dwOptions);
    if (FAILED(hr))
    {
        pfd->Release();
        return {};
    }

    dwOptions |= FOS_FILEMUSTEXIST | FOS_NOCHANGEDIR;

    if (multiSelect)
        dwOptions |= FOS_ALLOWMULTISELECT;
    else
        dwOptions &= ~FOS_ALLOWMULTISELECT;

    hr = pfd->SetOptions(dwOptions);
    if (FAILED(hr))
    {
        pfd->Release();
        return {};
    }

    std::vector<PlatformString> result;

    hr = pfd->Show(_window->GetHwnd());
    if (SUCCEEDED(hr))
        result = GetResults(pfd);

    pfd->Release();
    return result;
}

std::vector<PlatformString> PhotinoDialog::ShowOpenFolder(
    const PlatformString& title,
    const PlatformString& defaultPath,
    bool multiSelect) const
{
    assert(_window);
    if (!_window)
        return {};

    HRESULT hr = S_OK;
    IFileOpenDialog* pfd = CreateDlg<IFileOpenDialog>(&hr, title, defaultPath);
    if (!pfd)
        return {};

    DWORD dwOptions = 0;
    hr = pfd->GetOptions(&dwOptions);
    if (FAILED(hr))
    {
        pfd->Release();
        return {};
    }

    dwOptions |= FOS_PICKFOLDERS | FOS_NOCHANGEDIR;

    if (multiSelect)
        dwOptions |= FOS_ALLOWMULTISELECT;
    else
        dwOptions &= ~FOS_ALLOWMULTISELECT;

    hr = pfd->SetOptions(dwOptions);
    if (FAILED(hr))
    {
        pfd->Release();
        return {};
    }

    std::vector<PlatformString> result;

    hr = pfd->Show(_window->GetHwnd());
    if (SUCCEEDED(hr))
        result = GetResults(pfd);

    pfd->Release();
    return result;
}

PlatformString PhotinoDialog::ShowSaveFile(
    const PlatformString& title,
    const PlatformString& defaultPath,
    const std::vector<PlatformString>& filters,
    const PlatformString& defaultFileName) const
{
    assert(_window);
    if (!_window)
        return {};

    HRESULT hr = S_OK;
    IFileSaveDialog* pfd = CreateDlg<IFileSaveDialog>(&hr, title, defaultPath);
    if (!pfd)
        return {};

    if (!defaultFileName.empty())
    {
        hr = pfd->SetFileName(defaultFileName.c_str());
        if (FAILED(hr))
        {
            pfd->Release();
            return {};
        }
    }

    if (!AddFilters(pfd, filters))
    {
        pfd->Release();
        return {};
    }

    DWORD dwOptions = 0;
    hr = pfd->GetOptions(&dwOptions);
    if (FAILED(hr))
    {
        pfd->Release();
        return {};
    }

    dwOptions |= FOS_NOCHANGEDIR | FOS_OVERWRITEPROMPT;

    hr = pfd->SetOptions(dwOptions);
    if (FAILED(hr))
    {
        pfd->Release();
        return {};
    }

    PlatformString result;

    hr = pfd->Show(_window->GetHwnd());
    if (SUCCEEDED(hr))
    {
        IShellItem* psiResult = nullptr;
        hr = pfd->GetResult(&psiResult);
        if (SUCCEEDED(hr) && psiResult)
        {
            PWSTR pszName = nullptr;
            hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszName);

            if (pszName)
            {
                if (SUCCEEDED(hr))
                    result = pszName;

                CoTaskMemFree(pszName);
            }

            psiResult->Release();
        }
    }

    pfd->Release();
    return result;
}

DialogResult PhotinoDialog::ShowMessage(
    const PlatformString& title,
    const PlatformString& text,
    DialogButtons buttons,
    DialogIcon icon) const
{
    assert(_window);
    if (!_window)
        return DialogResult::Cancel;

    NewStyleContext ctx;

    UINT flags = {};

    switch (icon)
    {
    case DialogIcon::Info:
        flags |= MB_ICONINFORMATION;
        break;
    case DialogIcon::Warning:
        flags |= MB_ICONWARNING;
        break;
    case DialogIcon::Error:
        flags |= MB_ICONERROR;
        break;
    case DialogIcon::Question:
        flags |= MB_ICONQUESTION;
        break;
    }

    switch (buttons)
    {
    case DialogButtons::Ok:
        flags |= MB_OK;
        break;
    case DialogButtons::OkCancel:
        flags |= MB_OKCANCEL;
        break;
    case DialogButtons::YesNo:
        flags |= MB_YESNO;
        break;
    case DialogButtons::YesNoCancel:
        flags |= MB_YESNOCANCEL;
        break;
    case DialogButtons::RetryCancel:
        flags |= MB_RETRYCANCEL;
        break;
    case DialogButtons::AbortRetryIgnore:
        flags |= MB_ABORTRETRYIGNORE;
        break;
    default:
        flags |= MB_OK;
        break;
    }

    const auto result = MessageBoxW(_window->GetHwnd(), text.c_str(), title.c_str(), flags);

    switch (result)
    {
    case IDCANCEL:
        return DialogResult::Cancel;
    case IDOK:
        return DialogResult::Ok;
    case IDYES:
        return DialogResult::Yes;
    case IDNO:
        return DialogResult::No;
    case IDABORT:
        return DialogResult::Abort;
    case IDRETRY:
        return DialogResult::Retry;
    case IDIGNORE:
        return DialogResult::Ignore;
    default:
        return DialogResult::Cancel;
    }
}