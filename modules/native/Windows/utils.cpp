#include "../utils.h"

#if _WIN32
#include <windows.h>
#include <shobjidl.h>

#include <wrl.h>
using namespace Microsoft::WRL;

namespace eokas {
    
    static bool GetStringFromShellItem(String& strResult, const ComPtr<IShellItem>& pItem) {
        if (pItem.Get() != nullptr) {
            PWSTR pszWStr = NULL;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszWStr))) {
                strResult = String::unicodeToUtf8(pszWStr, false);
                CoTaskMemFree(pszWStr);
                return true;
            }
        }
        return false;
    }
    
    bool OpenFileDialog(String& selectedPath, const std::map<String, String>& filters) {
        ComPtr<IFileDialog> pFileDialog = NULL;
        if(FAILED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog)))) {
            return false;
        }
        
        DWORD dwOptions = 0;
        pFileDialog->GetOptions(&dwOptions);
        pFileDialog->SetOptions(dwOptions | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT);
        
        std::map<std::wstring, std::wstring> filtersW;
        for(auto& entry : filters) {
            std::wstring name = String::utf8ToUnicode(entry.first.cstr(), false);
            std::wstring spec = String::utf8ToUnicode(entry.second.cstr(), false);
            filtersW.insert(std::make_pair(name, spec));
        }
        
        std::vector<COMDLG_FILTERSPEC> filterTypes = {};
        for(auto& entry : filtersW) {
            auto& filter = filterTypes.emplace_back();
            filter.pszName = entry.first.c_str();
            filter.pszSpec = entry.second.c_str();
        }
        
        pFileDialog->SetFileTypes(filterTypes.size(), filterTypes.data());
        pFileDialog->SetFileTypeIndex(1);
        
        HWND hwndOwner = nullptr;
        pFileDialog->Show(hwndOwner);
        
        ComPtr<IShellItem> pItem = nullptr;
        pFileDialog->GetResult(&pItem);
        
        return GetStringFromShellItem(selectedPath, pItem);
    }
    
    bool OpenFolderDialog(String& selectedPath, const String& defaultPath) {
        ComPtr<IFileOpenDialog> pFileDialog = nullptr;
        if(FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog)))) {
            return false;
        }
        
        DWORD dwOptions;
        pFileDialog->GetOptions(&dwOptions);
        pFileDialog->SetOptions(dwOptions | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
        if(!defaultPath.isEmpty()) {
            std::wstring defaultPathW = String::utf8ToUnicode(defaultPath.cstr(), false);
            ComPtr<IShellItem> pDefaultFolderItem = nullptr;
            PCWSTR pszBaseLocation = defaultPathW.c_str();
            if(SUCCEEDED(SHCreateItemFromParsingName(pszBaseLocation, nullptr, IID_PPV_ARGS(&pDefaultFolderItem)))) {
                pFileDialog->SetDefaultFolder(pDefaultFolderItem.Get());
            }
        }
        
        HWND hwndOwner = nullptr;
        pFileDialog->Show(hwndOwner);
        
        ComPtr<IShellItem> pItem = nullptr;
        pFileDialog->GetResult(&pItem);
        
        return GetStringFromShellItem(selectedPath, pItem);
    }
}

#endif
