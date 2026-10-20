#ifndef _EOKAS_TOOL_BASE_MAIN_WINDOW_H_
#define _EOKAS_TOOL_BASE_MAIN_WINDOW_H_

#include "./MainMenu.h"
#include "Native/NativeWindow.h"
#include "Render/Renderer.h"
#include "Render/Space.h"
#include "UI/UIFont.h"
#include "UI/UIWindow.h"

#include <memory>

namespace eokas {

    class MainWindow {
    public:
        MainWindow();
        ~MainWindow();

        void setTitle(const char* title);
        void setSize(float width, float height);
        void setRenderer(Renderer& renderer);
        void tick(f32_t deltaTime);
        bool isOpen() const;
        void* handle() const;

        MainMenu& menu();
        const MainMenu& menu() const;
        UIWidget& content();
        const UIWidget& content() const;
        const Surface::Ref& surface() const;
        UIWindow& uiWindow();
        const UIWindow& uiWindow() const;

    private:
        class Frame;

        void ensureOpen(float width, float height);
        void closeNative();
        void bindNativeInput();
        void applyClientSize(float width, float height);
        void ensureSurface();
        void releaseSurface();
        void publishFont();

        Renderer* mRenderer = nullptr;
        Surface::Ref mSurface;
        Space mSpace;
        Camera::Ref mCamera;
        UIWindow mUIWindow;
        std::shared_ptr<Frame> mFrame;
        std::unique_ptr<UIFont> mFont;
        NativeWindow mNativeWindow;
        String mTitle;
        bool mUiReady = false;
        bool mAtlasReady = false;
    };

}

#endif//_EOKAS_TOOL_BASE_MAIN_WINDOW_H_
