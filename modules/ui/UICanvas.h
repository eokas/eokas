#ifndef _EOKAS_UI_CANVAS_H_
#define _EOKAS_UI_CANVAS_H_

#include "UIWidget.h"
#include "UIShape.h"
#include "UIFont.h"
#include "UIText.h"
#include <memory>
#include <vector>

namespace eokas
{
    class UICanvas
    {
    public:
        void init(uint32_t width, uint32_t height);
        void quit();

        const std::shared_ptr<UIWidget>& root() const;
        void setRoot(const std::shared_ptr<UIWidget>& widget);

        void prepare();
        UIFont* font();
        void setTexture(Texture::Ref texture, const std::vector<uint8_t>& rgba);

        void flush();
        UIShape::Ref shape() const;

        UIWidget* hitTest(float x, float y);
        void onMouseMove(float x, float y);
        void onMouseDown(float x, float y, int button);
        void onMouseUp(float x, float y, int button);
        void onMouseWheel(float x, float y, float deltaX, float deltaY);
        void onChar(uint32_t codepoint);
        void setFallbackFontPath(const char* path);
        void onKeyDown(UIKey key, const UIKeyMods& mods);
        void setFocus(UIWidget* widget);
        UIWidget* focus() const;

    private:
        String resolveAssetPath(const char* relativePath) const;
        void collectTexts(UIWidget* widget, std::vector<UIText*>& texts);
        UIFont* loadFont(const char* fontPath, uint32_t pixelSize);
        UIWidget* hitTestNode(UIWidget* widget, float x, float y, float originX, float originY);
        bool findWidget(UIWidget* node, UIWidget* target, float originX, float originY, float& outX, float& outY) const;
        void dispatchDrag(float x, float y);
        bool routeWheel(UIWidget* widget, float x, float y, float originX, float originY, float deltaX, float deltaY);
        void resetPointerState(UIWidget* widget);
        bool containsWidget(UIWidget* node, UIWidget* target) const;
        bool focusAlive();

        float mWidth = 0.0f;
        float mHeight = 0.0f;

        String mFallbackFontPath;
        std::shared_ptr<UIWidget> mRoot;
        std::vector<std::unique_ptr<UIFont>> mFonts;
        UIShape::Ref mShape;

        UIWidget* mHovered = nullptr;
        UIWidget* mPressed = nullptr;
        int mPressedButton = -1;
        UIWidget* mFocused = nullptr;
    };
}

#endif//_EOKAS_UI_CANVAS_H_
