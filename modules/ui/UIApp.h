#ifndef _EOKAS_UI_APP_H_
#define _EOKAS_UI_APP_H_

#include "UIWindow.h"
#include "widgets/UIDocking.h"
#include "UIFont.h"
#include <functional>
#include <memory>
#include <vector>

namespace eokas
{
    class UIText;

    class UIApp
    {
    public:
        UIApp() = default;
        ~UIApp();

        std::function<void*(const Rect& rect)> onCreateWindow;
        std::function<void(void* nativeWindow, const Rect& rect)> onPlaceWindow;
        std::function<void(void* nativeWindow)> onDestroyWindow;

        UIWindow& open(void* nativeWindow, uint32_t width, uint32_t height);
        void close(void* nativeWindow);
        UIWindow* find(void* nativeWindow);
        void setFallbackFontPath(const char* path);
        void prepare();
        void publishAtlas();
        void layout(void* nativeWindow, float width, float height);
        void flushClosing();

        void registerSpace(UIDockSpace* space);
        void unregisterSpace(UIDockSpace* space);
        void observe(const std::shared_ptr<UIDockPage>& page);
        void closeAll();

    private:
        struct Slot
        {
            void* nativeWindow = nullptr;
            std::unique_ptr<UIWindow> uiWindow;
            std::shared_ptr<UIDockPage> page;
            Rect screenRect;
        };

        std::vector<Slot> mWindows;
        std::vector<Slot> mFloating;
        std::vector<std::unique_ptr<UIWindow>> mClosing;
        std::vector<UIDockSpace*> mSpaces;
        std::vector<std::unique_ptr<UIFont>> mFonts;
        String mFallbackFontPath;
        UIDockPage* mDragPage = nullptr;
        UIDockSpace* mSourceSpace = nullptr;
        bool mDragging = false;
        bool mDragMoved = false;
        float mPressScreenX = 0.0f;
        float mPressScreenY = 0.0f;
        float mGrabScreenX = 0.0f;
        float mGrabScreenY = 0.0f;
        float mDragSlop = 4.0f;
        UIDockSpace* mPreviewSpace = nullptr;
        bool mHostDrag = false;
        bool mReleasing = false;

        std::vector<UIWindow*> liveWindows();
        Slot* slotOf(UIDockPage* page);
        void destroySlot(Slot& slot, bool deferWindow);
        void closeFonts();
        void toScreenPoint(UIDockPage* page, float x, float y, float& screenX, float& screenY);
        void beginFloat(UIDockPage* page, float screenX, float screenY);
        void updateFloat(UIDockPage* page, float screenX, float screenY);
        bool dragPage(UIDockPage* page, float x, float y, int button);
        void releasePage(UIDockPage* page);
        void collectTexts(UIWidget* widget, std::vector<UIText*>& texts);
        String resolveAssetPath(const char* relativePath) const;
        UIFont* loadFont(const char* fontPath, uint32_t pixelSize);
    };
}

#endif//_EOKAS_UI_APP_H_
