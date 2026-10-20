#ifndef _EOKAS_UI_HOST_H_
#define _EOKAS_UI_HOST_H_

#include "UI/UIWindow.h"
#include "UI/widgets/UIDocking.h"
#include "UI/UIFont.h"
#include <functional>
#include <memory>
#include <vector>

namespace eokas
{
    class UIText;

    class UIHost
    {
    public:
        UIHost() = default;
        ~UIHost();

        std::function<void(UIWindow& window, const Rect& rect)> onCreateWindow;
        std::function<void(UIWindow& window, const Rect& rect)> onPlaceWindow;
        std::function<void(UIWindow& window)> onDestroyWindow;

        UIWindow& open(uint32_t width, uint32_t height);
        void close(UIWindow& window);
        void setFallbackFontPath(const char* path);
        void prepare();
        void publishAtlas();
        void layout(UIWindow& window, float width, float height);
        void flushClosing();

        void registerSpace(UIDockSpace* space);
        void unregisterSpace(UIDockSpace* space);
        void observe(const std::shared_ptr<UIDockPage>& page);
        void closeAll();

    private:
        struct Slot
        {
            std::unique_ptr<UIWindow> uiWindow;
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
        void clientToScreen(UIDockPage* page, float x, float y, float& screenX, float& screenY);
        void beginFloat(UIDockPage* page, float screenX, float screenY);
        void updateFloat(UIDockPage* page, float screenX, float screenY);
        bool dragPage(UIDockPage* page, float x, float y, int button);
        void releasePage(UIDockPage* page);
        void collectTexts(UIWidget* widget, std::vector<UIText*>& texts);
        String resolveAssetPath(const char* relativePath) const;
        UIFont* loadFont(const char* fontPath, uint32_t pixelSize);
    };
}

#endif//_EOKAS_UI_HOST_H_
