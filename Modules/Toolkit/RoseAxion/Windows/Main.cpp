#include "../App.h"

#include "Base/IO.h"
#include "Base/Time.h"
#include "Base/Timer.h"
#include "Native/NativeWindow.h"
#include "Render/Camera.h"
#include "Render/Renderer.h"
#include "Render/Space.h"
#include "UI/UIFont.h"
#include "UI/UIWindow.h"

#include <sdkddkver.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include <dbghelp.h>
#include <cstdio>
#include <exception>
#include <memory>
#include <string>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "shell32.lib")

static void writeCrashText(HANDLE file, const char* text)
{
    if (file == INVALID_HANDLE_VALUE || text == nullptr)
    {
        return;
    }
    DWORD written = 0;
    WriteFile(file, text, static_cast<DWORD>(lstrlenA(text)), &written, nullptr);
}

static LONG WINAPI roseAxionCrashFilter(EXCEPTION_POINTERS* info)
{
    HANDLE file = CreateFileA(
        "E:\\Cheelin\\Projects\\eokas\\eokas\\roseaxion-crash.txt",
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    char line[1024];
    snprintf(
        line,
        sizeof(line),
        "code=%08lX addr=%p\r\n",
        info != nullptr && info->ExceptionRecord != nullptr ? info->ExceptionRecord->ExceptionCode : 0,
        info != nullptr && info->ExceptionRecord != nullptr ? info->ExceptionRecord->ExceptionAddress : nullptr);
    writeCrashText(file, line);
    if (info != nullptr && info->ContextRecord != nullptr)
    {
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        if (SymInitialize(process, nullptr, TRUE))
        {
            CONTEXT context = *info->ContextRecord;
            STACKFRAME64 frame = {};
            frame.AddrPC.Offset = context.Rip;
            frame.AddrPC.Mode = AddrModeFlat;
            frame.AddrFrame.Offset = context.Rbp;
            frame.AddrFrame.Mode = AddrModeFlat;
            frame.AddrStack.Offset = context.Rsp;
            frame.AddrStack.Mode = AddrModeFlat;
            for (int i = 0; i < 48; ++i)
            {
                if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
                {
                    break;
                }
                if (frame.AddrPC.Offset == 0)
                {
                    break;
                }
                char symbolBuffer[sizeof(SYMBOL_INFO) + 256] = {};
                auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
                symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol->MaxNameLen = 255;
                DWORD64 displacement = 0;
                if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
                {
                    IMAGEHLP_LINE64 source = {};
                    source.SizeOfStruct = sizeof(source);
                    DWORD lineDisplacement = 0;
                    if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &source))
                    {
                        snprintf(line, sizeof(line), "%s + 0x%llX\r\n  %s:%lu\r\n", symbol->Name, static_cast<unsigned long long>(displacement), source.FileName, source.LineNumber);
                    }
                    else
                    {
                        snprintf(line, sizeof(line), "%s + 0x%llX\r\n", symbol->Name, static_cast<unsigned long long>(displacement));
                    }
                }
                else
                {
                    snprintf(line, sizeof(line), "%p\r\n", reinterpret_cast<void*>(frame.AddrPC.Offset));
                }
                writeCrashText(file, line);
            }
            SymCleanup(process);
        }
    }
    if (file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

namespace {

    eokas::String fromWide(const wchar_t* text)
    {
        if (text == nullptr || text[0] == L'\0')
        {
            return eokas::String::empty;
        }
        eokas::MBString utf8 = eokas::String::unicodeToUtf8(eokas::WCString(text), false);
        return eokas::String(utf8.c_str());
    }

    eokas::WCString toWide(const eokas::String& text)
    {
        return eokas::String::utf8ToUnicode(eokas::MBString(text.cstr(), text.length()), false);
    }

    eokas::String pickOpen(HWND owner)
    {
        wchar_t buffer[MAX_PATH] = {};
        OPENFILENAMEW dialog = {};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = owner;
        dialog.lpstrFilter = L"Rose Library (*.rose)\0*.rose\0";
        dialog.lpstrFile = buffer;
        dialog.nMaxFile = MAX_PATH;
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
        dialog.lpstrDefExt = L"rose";
        if (!GetOpenFileNameW(&dialog))
        {
            return eokas::String::empty;
        }
        return fromWide(buffer);
    }

    eokas::String pickSave(HWND owner, const eokas::String& suggested)
    {
        wchar_t buffer[MAX_PATH] = {};
        eokas::WCString wide = toWide(suggested);
        if (!wide.empty())
        {
            wcsncpy_s(buffer, wide.c_str(), _TRUNCATE);
        }
        OPENFILENAMEW dialog = {};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = owner;
        dialog.lpstrFilter = L"Rose Library (*.rose)\0*.rose\0";
        dialog.lpstrFile = buffer;
        dialog.nMaxFile = MAX_PATH;
        dialog.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;
        dialog.lpstrDefExt = L"rose";
        if (!GetSaveFileNameW(&dialog))
        {
            return eokas::String::empty;
        }
        return fromWide(buffer);
    }

    void writeClipboard(const eokas::String& value)
    {
        if (!OpenClipboard(nullptr))
        {
            return;
        }
        EmptyClipboard();
        eokas::WCString wide = toWide(value);
        size_t bytes = (wide.size() + 1) * sizeof(wchar_t);
        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (mem != nullptr)
        {
            void* ptr = GlobalLock(mem);
            if (ptr != nullptr)
            {
                memcpy(ptr, wide.c_str(), bytes);
                GlobalUnlock(mem);
                SetClipboardData(CF_UNICODETEXT, mem);
            }
            else
            {
                GlobalFree(mem);
            }
        }
        CloseClipboard();
    }

    eokas::String readClipboard()
    {
        eokas::String result;
        if (!OpenClipboard(nullptr))
        {
            return result;
        }
        HANDLE mem = GetClipboardData(CF_UNICODETEXT);
        if (mem != nullptr)
        {
            const wchar_t* ptr = static_cast<const wchar_t*>(GlobalLock(mem));
            if (ptr != nullptr)
            {
                result = fromWide(ptr);
                GlobalUnlock(mem);
            }
        }
        CloseClipboard();
        return result;
    }

    void openExternal(const eokas::String& url)
    {
        eokas::WCString wide = toWide(url);
        ShellExecuteW(nullptr, L"open", wide.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    bool ctrlDown()
    {
        return (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    }

    eokas::String findFont()
    {
        const char* paths[] = {
            "C:/Windows/Fonts/msyh.ttc",
            "C:/Windows/Fonts/msyh.ttf",
            "C:/Windows/Fonts/simhei.ttf",
            "C:/Windows/Fonts/segoeui.ttf",
        };
        for (const char* path : paths)
        {
            if (eokas::File::exists(path))
            {
                return path;
            }
        }
        return eokas::String::empty;
    }

    class Host
    {
    public:
        bool init()
        {
            const float logicalWidth = 1280.0f;
            const float logicalHeight = 800.0f;
            if (!mNative.create(eokas::String("RoseAxion"), logicalWidth, logicalHeight))
            {
                return false;
            }
            HWND hwnd = static_cast<HWND>(mNative.handle());
            UINT dpi = GetDpiForWindow(hwnd);
            if (dpi == 0)
            {
                dpi = 96;
            }
            mScale = static_cast<float>(dpi) / 96.0f;
            mNative.setClientSize(logicalWidth * mScale, logicalHeight * mScale);
            mNative.center();
            eokas::Vector2 client = mNative.clientSize();
            mWidth = client.x >= 1.0f ? client.x : logicalWidth * mScale;
            mHeight = client.y >= 1.0f ? client.y : logicalHeight * mScale;

            mRenderer.create();
            mSurface = mRenderer.attach(hwnd, static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight));
            mWindow.init(static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight));

            mFontPath = findFont();
            if (mFontPath.isEmpty())
            {
                return false;
            }
            mFont = std::make_unique<eokas::UIFont>();
            if (!mFont->open(mFontPath.cstr(), 16))
            {
                return false;
            }
            mFont->prepareSize(static_cast<uint32_t>(15.0f * mScale + 0.5f));
            mFont->prepareSize(static_cast<uint32_t>(13.0f * mScale + 0.5f));
            mFont->prepareSize(static_cast<uint32_t>(17.0f * mScale + 0.5f));
            eokas::UIFont::bind(mFontPath, mFont.get());
            this->uploadFont();

            eokas::RoseAxion::AxionHost host;
            host.pickOpenPath = [hwnd]() { return pickOpen(hwnd); };
            host.pickSavePath = [hwnd](const eokas::String& suggested) { return pickSave(hwnd, suggested); };
            host.writeClipboard = [](const eokas::String& value) { writeClipboard(value); };
            host.readClipboard = []() { return readClipboard(); };
            host.openExternal = [](const eokas::String& url) { openExternal(url); };
            host.ctrlDown = []() { return ctrlDown(); };
            host.setTitle = [this](const eokas::String& title) { mNative.setTitle(title); };
            mApp.attach(mWindow, host, mFontPath, mScale);
            mApp.resize(mWidth, mHeight);

            auto camera = std::make_shared<eokas::Camera>();
            camera->viewport.left = 0.0f;
            camera->viewport.top = 0.0f;
            camera->viewport.right = mWidth;
            camera->viewport.bottom = mHeight;
            camera->viewport.front = 0.0f;
            camera->viewport.back = 1.0f;
            mCamera = camera;
            mSpace.clearColor = eokas::Color(0.102f, 0.110f, 0.133f, 1.0f);
            mSpace.add(camera);
            mSpace.activeCamera = camera;
            mSpace.add(mWindow.primitive());
            this->bindInput();
            mApp.tick();
            mWindow.tick(0.0f);
            this->uploadFont();
            mNative.show();
            return true;
        }

        void quit()
        {
            mRenderer.detach(mSurface);
            mSurface.reset();
            mWindow.quit();
            mFont.reset();
            mNative.destroy();
        }

        bool isOpen() const
        {
            return mRunning && mNative.isOpen();
        }

        void pump()
        {
            mNative.pumpMessages();
        }

        void tick(float delta)
        {
            if (!mRunning)
            {
                return;
            }
            mApp.tick();
            this->uploadFont();
            mWindow.tick(delta);
            this->uploadFont();
            if (mSurface)
            {
                mRenderer.render(mSurface, mSpace);
            }
        }

    private:
        void uploadFont()
        {
            if (!mFont || !mWindow.primitive())
            {
                return;
            }
            if (mFont->takeAtlasDirty())
            {
                mWindow.primitive()->setPendingUpload(mFont->atlasRgba(), mFont->atlasSize());
            }
        }

        void bindInput()
        {
            mNative.onClose = [this]()
            {
                mRunning = false;
            };
            mNative.onClientSize = [this](float width, float height)
            {
                if (width < 1.0f || height < 1.0f)
                {
                    return;
                }
                mWidth = width;
                mHeight = height;
                if (mSurface)
                {
                    mRenderer.resize(mSurface, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
                }
                if (mCamera)
                {
                    mCamera->viewport.right = width;
                    mCamera->viewport.bottom = height;
                }
                mApp.resize(width, height);
            };
            mNative.onMouseMove = [this](float x, float y)
            {
                mWindow.onMouseMove(x, y);
            };
            mNative.onMouseDown = [this](float x, float y, eokas::NativeMouseButton button)
            {
                mWindow.onMouseDown(x, y, static_cast<eokas::UIPointerCode>(static_cast<int>(button)));
            };
            mNative.onMouseUp = [this](float x, float y, eokas::NativeMouseButton button)
            {
                mWindow.onMouseUp(x, y, static_cast<eokas::UIPointerCode>(static_cast<int>(button)));
            };
            mNative.onMouseWheel = [this](float x, float y, float deltaX, float deltaY)
            {
                mWindow.onMouseWheel(x, y, deltaX, deltaY);
            };
            mNative.onKeyPress = [this](eokas::NativeKeyCode keyCode, bool, bool ctrl, bool shift, bool alt, bool repeat)
            {
                eokas::UIKeyEvent event;
                event.key = static_cast<eokas::UIKeyCode>(static_cast<int>(keyCode));
                event.ctrl = ctrl;
                event.shift = shift;
                event.alt = alt;
                event.repeat = repeat;
                mWindow.onKeyPress(event);
                return true;
            };
            mNative.onKeyChar = [this](uint32_t character)
            {
                mWindow.onKeyChar(character);
                return true;
            };
            mNative.onKeyRelease = [this](eokas::NativeKeyCode keyCode, bool, bool ctrl, bool shift, bool alt)
            {
                eokas::UIKeyEvent event;
                event.key = static_cast<eokas::UIKeyCode>(static_cast<int>(keyCode));
                event.ctrl = ctrl;
                event.shift = shift;
                event.alt = alt;
                mWindow.onKeyRelease(event);
                return true;
            };
        }

        eokas::NativeWindow mNative;
        eokas::Renderer mRenderer;
        eokas::Surface::Ref mSurface;
        eokas::Space mSpace;
        eokas::Camera::Ref mCamera;
        eokas::UIWindow mWindow;
        std::unique_ptr<eokas::UIFont> mFont;
        eokas::String mFontPath;
        eokas::RoseAxion::AxionApp mApp;
        float mWidth = 1280.0f;
        float mHeight = 800.0f;
        float mScale = 1.0f;
        bool mRunning = true;
    };

}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    SetUnhandledExceptionFilter(roseAxionCrashFilter);
    try
    {
        Host host;
        if (!host.init())
        {
            MessageBoxW(nullptr, L"RoseAxion 启动失败。", L"RoseAxion", MB_OK | MB_ICONERROR);
            return 1;
        }
        eokas::Timer timer;
        while (host.isOpen())
        {
            host.pump();
            if (!host.isOpen())
            {
                break;
            }
            float delta = static_cast<float>(eokas::TimeSpan(timer.elapse()).exactSeconds());
            host.tick(delta);
        }
        host.quit();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::string text = error.what();
        MessageBoxA(nullptr, text.c_str(), "RoseAxion", MB_OK | MB_ICONERROR);
        return 1;
    }
    catch (...)
    {
        MessageBoxW(nullptr, L"RoseAxion 发生未知错误。", L"RoseAxion", MB_OK | MB_ICONERROR);
        return 1;
    }
}
