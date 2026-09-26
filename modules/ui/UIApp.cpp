#include "UIApp.h"
#include "widgets/UIText.h"
#include "widgets/UIView.h"

#include <map>
#include <stdexcept>

namespace eokas
{
    UIApp::~UIApp()
    {
        this->closeAll();
        for (auto& slot : mWindows)
        {
            if (slot.frame)
            {
                slot.frame->quit();
            }
        }
        mWindows.clear();
        this->flushClosing();
        this->closeFonts();
    }

    UIFrame& UIApp::open(void* window, uint32_t width, uint32_t height)
    {
        Slot slot;
        slot.window = window;
        slot.frame = std::make_unique<UIFrame>();
        slot.frame->init(width, height);
        mWindows.push_back(std::move(slot));
        return *mWindows.back().frame;
    }

    void UIApp::close(void* window)
    {
        for (auto it = mWindows.begin(); it != mWindows.end(); ++it)
        {
            if (it->window != window)
            {
                continue;
            }
            if (it->frame)
            {
                it->frame->quit();
            }
            mWindows.erase(it);
            return;
        }
    }

    UIFrame* UIApp::find(void* window)
    {
        for (auto& slot : mWindows)
        {
            if (slot.window == window && slot.frame)
            {
                return slot.frame.get();
            }
        }
        for (auto& slot : mFloating)
        {
            if (slot.window == window && slot.frame)
            {
                return slot.frame.get();
            }
        }
        return nullptr;
    }

    void UIApp::setFallbackFontPath(const char* path)
    {
        mFallbackFontPath = path != nullptr ? path : "";
    }

    void UIApp::prepare()
    {
        this->closeFonts();

        std::vector<UIText*> texts;
        for (UIFrame* frame : this->liveFrames())
        {
            if (frame == nullptr || !frame->root())
            {
                continue;
            }
            this->collectTexts(frame->root().get(), texts);
        }

        std::map<String, std::vector<UIText*>> groups;
        for (UIText* text : texts)
        {
            if (text == nullptr || text->style.fontPath.isEmpty())
            {
                continue;
            }
            groups[text->style.fontPath].push_back(text);
        }

        for (auto& entry : groups)
        {
            uint32_t pixelSize = 0;
            std::map<uint32_t, uint32_t> sizes;
            for (UIText* text : entry.second)
            {
                uint32_t px = (uint32_t)(text->style.fontSize + 0.5f);
                if (px < 1) px = 1;
                sizes[px] = px;
                if (px > pixelSize) pixelSize = px;
            }
            if (pixelSize == 0) pixelSize = (uint32_t)UIText::kDefaultFontSize;

            UIFont* font = this->loadFont(entry.first.cstr(), pixelSize);
            for (auto& size : sizes) font->prepareSize(size.first);
        }

        if (mFonts.empty())
        {
            return;
        }
        UIFont* font = mFonts.front().get();
        for (UIFrame* frame : this->liveFrames())
        {
            if (frame == nullptr)
            {
                continue;
            }
            if (UIPrimitive::Ref primitive = frame->primitive())
            {
                primitive->setPendingUpload(font->atlasRgba(), font->atlasSize());
            }
        }
    }

    void UIApp::publishAtlas()
    {
        std::vector<UIFont*> dirty;
        for (auto& font : mFonts)
        {
            if (font && font->takeAtlasDirty())
            {
                dirty.push_back(font.get());
            }
        }
        if (dirty.empty())
        {
            return;
        }
        std::vector<UIFrame*> frames = this->liveFrames();
        for (UIFont* font : dirty)
        {
            for (UIFrame* frame : frames)
            {
                if (frame == nullptr)
                {
                    continue;
                }
                if (UIPrimitive::Ref primitive = frame->primitive())
                {
                    primitive->setPendingUpload(font->atlasRgba(), font->atlasSize());
                }
            }
        }
    }

    void UIApp::layout(void* window, float width, float height)
    {
        for (auto& slot : mFloating)
        {
            if (slot.window != window || !slot.page)
            {
                continue;
            }
            slot.page->layoutInWindow(width, height);
            return;
        }
    }

    void UIApp::flushClosing()
    {
        auto closing = std::move(mClosing);
        mClosing.clear();
        for (auto& frame : closing)
        {
            if (frame)
            {
                frame->quit();
            }
        }
    }

    void UIApp::registerSpace(UIDockSpace* space)
    {
        if (!space) return;
        for (auto* item : mSpaces) if (item == space) return;
        space->onDestroy = [this](UIDockSpace& closing) { this->unregisterSpace(&closing); };
        mSpaces.push_back(space);
    }

    void UIApp::unregisterSpace(UIDockSpace* space)
    {
        for (auto it = mSpaces.begin(); it != mSpaces.end(); ++it)
        {
            if (*it != space) continue;
            mSpaces.erase(it);
            break;
        }
        if (mPreviewSpace == space) mPreviewSpace = nullptr;
        if (mSourceSpace == space) mSourceSpace = nullptr;
    }

    void UIApp::observe(const std::shared_ptr<UIDockPage>& page)
    {
        if (!page) return;
        page->onDrag = [this](UIDockPage& target, float x, float y, int button)
        {
            return this->dragPage(&target, x, y, button);
        };
        page->onDragEnd = [this](UIDockPage& target) { this->releasePage(&target); };
    }

    void UIApp::closeAll()
    {
        mDragging = false;
        mDragPage = nullptr;
        mSourceSpace = nullptr;
        mPreviewSpace = nullptr;
        for (auto& slot : mFloating) this->destroySlot(slot, false);
        mFloating.clear();
    }

    std::vector<UIFrame*> UIApp::liveFrames()
    {
        std::vector<UIFrame*> frames;
        for (auto& slot : mWindows)
        {
            if (slot.frame) frames.push_back(slot.frame.get());
        }
        for (auto& slot : mFloating)
        {
            if (slot.frame) frames.push_back(slot.frame.get());
        }
        return frames;
    }

    UIApp::Slot* UIApp::slotOf(UIDockPage* page)
    {
        for (auto& slot : mFloating) if (slot.page.get() == page) return &slot;
        return nullptr;
    }

    void UIApp::destroySlot(Slot& slot, bool deferFrame)
    {
        void* window = slot.window;
        slot.window = nullptr;
        if (slot.frame)
        {
            slot.frame->setRoot(nullptr);
            if (deferFrame)
            {
                mClosing.push_back(std::move(slot.frame));
            }
            else
            {
                slot.frame->quit();
            }
        }
        slot.page.reset();
        if (window && onDestroyWindow) onDestroyWindow(window);
    }

    void UIApp::closeFonts()
    {
        for (auto& font : mFonts)
        {
            if (font)
            {
                font->close();
            }
        }
        mFonts.clear();
    }

    void UIApp::toScreenPoint(UIDockPage* page, float x, float y, float& screenX, float& screenY)
    {
        if (mSourceSpace)
        {
            Vector2 point(x, y);
            if (page && page->space() == mSourceSpace)
            {
                point += Vector2(mSourceSpace->shape.left(), mSourceSpace->shape.top()) + Vector2(page->shape.left(), page->shape.top());
            }
            Rect screen = mSourceSpace->toScreen(Rect(point, Vector2::ZERO));
            screenX = screen.origin.x;
            screenY = screen.origin.y;
            return;
        }
        if (Slot* slot = this->slotOf(page))
        {
            screenX = slot->screenRect.origin.x + x;
            screenY = slot->screenRect.origin.y + y;
            return;
        }
        screenX = x;
        screenY = y;
    }

    void UIApp::beginFloat(UIDockPage* page, float screenX, float screenY)
    {
        if (!page) return;
        if (Slot* slot = this->slotOf(page))
        {
            mGrabScreenX = screenX - slot->screenRect.origin.x;
            mGrabScreenY = screenY - slot->screenRect.origin.y;
            return;
        }
        Rect window(screenX, screenY, 320.0f, 240.0f);
        std::shared_ptr<UIDockPage> held;
        if (page->space())
        {
            UIDockSpace* space = page->space();
            space->layout(Rect(space->shape.left(), space->shape.top(), space->shape.size.x, space->shape.size.y));
            Rect local;
            if (space->pageBounds(page, local)) window = space->toScreen(local);
            held = space->takePage(page);
        }
        if (!held) return;
        void* windowHandle = nullptr;
        if (onCreateWindow) windowHandle = onCreateWindow(window);
        auto frame = std::make_unique<UIFrame>();
        uint32_t width = (uint32_t)window.size.x;
        uint32_t height = (uint32_t)window.size.y;
        if (width < 1) width = 1;
        if (height < 1) height = 1;
        frame->init(width, height);
        held->layoutInWindow((float)width, (float)height);
        frame->setRoot(held);
        mGrabScreenX = screenX - window.origin.x;
        mGrabScreenY = screenY - window.origin.y;
        Slot created;
        created.window = windowHandle;
        created.frame = std::move(frame);
        created.page = held;
        created.screenRect = window;
        mFloating.push_back(std::move(created));
        this->prepare();
        if (windowHandle && onPlaceWindow) onPlaceWindow(windowHandle, window);
    }

    void UIApp::updateFloat(UIDockPage* page, float screenX, float screenY)
    {
        Slot* slot = this->slotOf(page);
        if (!slot) return;
        slot->screenRect.origin.x = screenX - mGrabScreenX;
        slot->screenRect.origin.y = screenY - mGrabScreenY;
        if (slot->window && onPlaceWindow) onPlaceWindow(slot->window, slot->screenRect);
        UIDockSpace* hit = nullptr;
        for (auto* space : mSpaces)
        {
            if (!space) continue;
            Rect screen = space->toScreen(Rect(space->shape.left(), space->shape.top(), space->shape.size.x, space->shape.size.y));
            if (!screen.contains(Vector2(screenX, screenY))) continue;
            float localX = space->shape.left() + (screenX - screen.origin.x);
            float localY = space->shape.top() + (screenY - screen.origin.y);
            if (space->showPreview(localX, localY)) hit = space;
            else space->clearPreview();
        }
        if (mPreviewSpace && mPreviewSpace != hit) mPreviewSpace->clearPreview();
        mPreviewSpace = hit;
    }

    bool UIApp::dragPage(UIDockPage* page, float x, float y, int button)
    {
        if (button != 0 || !page) return false;
        if (!mDragging || mDragPage != page)
        {
            mDragging = true;
            mDragPage = page;
            mDragMoved = false;
            mSourceSpace = page->space();
            mDragSlop = mSourceSpace ? mSourceSpace->dragSlop : 4.0f;
            this->toScreenPoint(page, x, y, mPressScreenX, mPressScreenY);
        }
        float screenX = 0.0f;
        float screenY = 0.0f;
        this->toScreenPoint(page, x, y, screenX, screenY);
        float dx = screenX - mPressScreenX;
        float dy = screenY - mPressScreenY;
        if (!mDragMoved && (dx * dx + dy * dy) < mDragSlop * mDragSlop) return false;
        if (!mDragMoved) this->beginFloat(page, screenX, screenY);
        mDragMoved = true;
        this->updateFloat(page, screenX, screenY);
        return true;
    }

    void UIApp::releasePage(UIDockPage* page)
    {
        if (mDragPage != page) return;
        mDragging = false;
        mDragPage = nullptr;
        mSourceSpace = nullptr;
        Slot* slot = this->slotOf(page);
        if (mDragMoved && mPreviewSpace && slot)
        {
            auto held = slot->page;
            void* window = slot->window;
            std::unique_ptr<UIFrame> frame = std::move(slot->frame);
            UIDockSpace* space = mPreviewSpace;
            mPreviewSpace = nullptr;
            for (auto it = mFloating.begin(); it != mFloating.end(); ++it)
            {
                if (it->page.get() != page) continue;
                mFloating.erase(it);
                break;
            }
            space->acceptDrop(held);
            if (frame)
            {
                frame->setRoot(nullptr);
                mClosing.push_back(std::move(frame));
            }
            this->prepare();
            if (window && onDestroyWindow) onDestroyWindow(window);
        }
        else if (mPreviewSpace)
        {
            mPreviewSpace->clearPreview();
            mPreviewSpace = nullptr;
        }
        mDragMoved = false;
    }

    void UIApp::collectTexts(UIWidget* widget, std::vector<UIText*>& texts)
    {
        if (widget == nullptr)
        {
            return;
        }

        UIText* text = dynamic_cast<UIText*>(widget);
        if (text != nullptr)
        {
            texts.push_back(text);
        }

        for (auto& child : widget->children)
        {
            this->collectTexts(child.get(), texts);
        }
        if (UIView* view = dynamic_cast<UIView*>(widget))
        {
            for (auto& child : view->root()->children)
            {
                this->collectTexts(child.get(), texts);
            }
        }
    }

    String UIApp::resolveAssetPath(const char* relativePath) const
    {
        String given = relativePath;
        if (File::exists(given))
        {
            return given;
        }

        String exeDir = File::basePath(Process::executingPath());
        String fromExe = File::combinePath(exeDir, relativePath);
        if (File::exists(fromExe))
        {
            return fromExe;
        }

        String fromBuildDir = File::combinePath(File::basePath(exeDir), relativePath);
        if (File::exists(fromBuildDir))
        {
            return fromBuildDir;
        }

        return given;
    }

    UIFont* UIApp::loadFont(const char* fontPath, uint32_t pixelSize)
    {
        String path = this->resolveAssetPath(fontPath);
        auto font = std::make_unique<UIFont>();
        if (!font->open(path.cstr(), pixelSize))
        {
            throw std::runtime_error("Failed to load UI font.");
        }

        if (!mFallbackFontPath.isEmpty())
        {
            String fallback = this->resolveAssetPath(mFallbackFontPath.cstr());
            font->attachFallback(fallback.cstr());
        }

        UIFont* ptr = font.get();
        UIFont::bind(fontPath, ptr);
        mFonts.push_back(std::move(font));
        return ptr;
    }
}
