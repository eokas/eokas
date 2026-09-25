#ifndef _EOKAS_UI_APP_H_
#define _EOKAS_UI_APP_H_

#include "UICanvas.h"
#include "UIDocking.h"
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
        std::function<void(void* window, const Rect& rect)> onPlaceWindow;
        std::function<void(void* window)> onDestroyWindow;

        UICanvas& open(void* window, uint32_t width, uint32_t height);
        void close(void* window);
        UICanvas* find(void* window);
        void setFallbackFontPath(const char* path);
        void prepare();
        void publishAtlas();
        void layout(void* window, float width, float height);
        void flushClosing();

        void registerSpace(UIDockSpace* space);
        void unregisterSpace(UIDockSpace* space);
        void observe(const std::shared_ptr<UIDockPage>& page);
        void closeAll();

    private:
        struct Slot
        {
            void* window = nullptr;
            std::unique_ptr<UICanvas> canvas;
            std::shared_ptr<UIDockPage> page;
            Rect screenRect;
        };

        std::vector<Slot> mWindows;
        std::vector<Slot> mFloating;
        std::vector<std::unique_ptr<UICanvas>> mClosing;
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

        std::vector<UICanvas*> liveCanvases();
        Slot* slotOf(UIDockPage* page);
        void destroySlot(Slot& slot, bool deferCanvas);
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
