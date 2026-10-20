#include "../NativeDialogs.h"

#if _WIN32
#include <windows.h>
#include <objbase.h>
#include <shobjidl.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

namespace eokas
{
    namespace
    {
        struct ComApartment
        {
            bool release = false;

            ComApartment()
            {
                HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                release = result == S_OK || result == S_FALSE;
            }

            ~ComApartment()
            {
                if (release) {
                    CoUninitialize();
                }
            }
        };

        HWND ownerWindow(void* owner)
        {
            return owner ? static_cast<HWND>(owner) : nullptr;
        }

        bool GetStringFromShellItem(String& strResult, const ComPtr<IShellItem>& pItem)
        {
            if (pItem.Get() == nullptr) {
                return false;
            }
            PWSTR path = nullptr;
            if (FAILED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &path)) || path == nullptr) {
                return false;
            }
            strResult = String::unicodeToUtf8(path, false);
            CoTaskMemFree(path);
            return true;
        }

        struct FilterText
        {
            std::wstring name;
            std::wstring spec;
        };

        void buildFileTypes(const std::map<String, String>& filters, std::vector<FilterText>& texts, std::vector<COMDLG_FILTERSPEC>& specs)
        {
            texts.clear();
            specs.clear();
            if (filters.empty()) {
                return;
            }

            texts.reserve(filters.size());
            for (const auto& entry : filters) {
                FilterText text;
                const char* name = entry.first.cstr();
                const char* spec = entry.second.cstr();
                text.name = name ? String::utf8ToUnicode(name, false) : std::wstring();
                text.spec = spec ? String::utf8ToUnicode(spec, false) : std::wstring();
                texts.push_back(std::move(text));
            }

            specs.reserve(texts.size());
            for (const FilterText& text : texts) {
                COMDLG_FILTERSPEC spec = {};
                spec.pszName = text.name.c_str();
                spec.pszSpec = text.spec.c_str();
                specs.push_back(spec);
            }
        }
    }

    void NativeDialogs::alert(const String& title, const String& message, void* owner)
    {
        const char* titleUtf8 = title.cstr();
        const char* messageUtf8 = message.cstr();
        std::wstring titleW = titleUtf8 ? String::utf8ToUnicode(titleUtf8, false) : std::wstring();
        std::wstring messageW = messageUtf8 ? String::utf8ToUnicode(messageUtf8, false) : std::wstring();
        MessageBoxW(ownerWindow(owner), messageW.c_str(), titleW.c_str(), MB_OK | MB_ICONINFORMATION);
    }

    int NativeDialogs::confirm(const String& title, const String& message, void* owner)
    {
        const char* titleUtf8 = title.cstr();
        const char* messageUtf8 = message.cstr();
        std::wstring titleW = titleUtf8 ? String::utf8ToUnicode(titleUtf8, false) : std::wstring();
        std::wstring messageW = messageUtf8 ? String::utf8ToUnicode(messageUtf8, false) : std::wstring();
        int result = MessageBoxW(ownerWindow(owner), messageW.c_str(), titleW.c_str(), MB_YESNOCANCEL | MB_ICONQUESTION);
        if (result == IDYES) return 1;
        if (result == IDNO) return -1;
        return 0;
    }

    bool NativeDialogs::openFileDialog(String& selectedPath, const std::map<String, String>& filters, void* owner)
    {
        ComApartment apartment;

        ComPtr<IFileDialog> dialog;
        if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
            return false;
        }

        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);

        std::vector<FilterText> texts;
        std::vector<COMDLG_FILTERSPEC> specs;
        buildFileTypes(filters, texts, specs);
        if (!specs.empty()) {
            dialog->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
            dialog->SetFileTypeIndex(1);
        }

        HRESULT shown = dialog->Show(ownerWindow(owner));
        if (FAILED(shown)) {
            return false;
        }

        ComPtr<IShellItem> item;
        if (FAILED(dialog->GetResult(&item))) {
            return false;
        }
        return GetStringFromShellItem(selectedPath, item);
    }

    bool NativeDialogs::openFolderDialog(String& selectedPath, const String& defaultPath, void* owner)
    {
        ComApartment apartment;

        ComPtr<IFileOpenDialog> dialog;
        if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
            return false;
        }

        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
        if (!defaultPath.isEmpty()) {
            const char* utf8 = defaultPath.cstr();
            std::wstring defaultPathW = utf8 ? String::utf8ToUnicode(utf8, false) : std::wstring();
            ComPtr<IShellItem> defaultFolder;
            if (SUCCEEDED(SHCreateItemFromParsingName(defaultPathW.c_str(), nullptr, IID_PPV_ARGS(&defaultFolder)))) {
                dialog->SetDefaultFolder(defaultFolder.Get());
            }
        }

        HRESULT shown = dialog->Show(ownerWindow(owner));
        if (FAILED(shown)) {
            return false;
        }

        ComPtr<IShellItem> item;
        if (FAILED(dialog->GetResult(&item))) {
            return false;
        }
        return GetStringFromShellItem(selectedPath, item);
    }
}

#endif
