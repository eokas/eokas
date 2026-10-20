#include "UIHost.h"
#include "UI/widgets/UIText.h"

#include <map>
#include <stdexcept>

namespace
{
    struct ReleaseGuard
    {
        bool& flag;
        explicit ReleaseGuard(bool& value) : flag(value) { flag = true; }
        ~ReleaseGuard() { flag = false; }
    };
}

namespace eokas
{
    UIHost::~UIHost()
    {
        this->closeAll();
        for (auto& slot : mWindows)
        {
            if (slot.uiWindow)
            {
                slot.uiWindow->quit();
            }
        }
        mWindows.clear();
        this->flushClosing();
        this->closeFonts();
    }

    UIWindow& UIHost::open(uint32_t width, uint32_t height)
    {
        Slot slot;
        slot.uiWindow = std::make_unique<UIWindow>();
        slot.uiWindow->init(width, height);
        mWindows.push_back(std::move(slot));
        return *mWindows.back().uiWindow;
    }

    void UIHost::close(UIWindow& window)
    {
        for (auto it = mWindows.begin(); it != mWindows.end(); ++it)
        {
            if (!it->uiWindow || it->uiWindow.get() != &window)
            {
                continue;
            }
            it->uiWindow->quit();
            mWindows.erase(it);
            return;
        }
    }

    void UIHost::setFallbackFontPath(const char* path)
    {
        mFallbackFontPath = path != nullptr ? path : "";
    }

    void UIHost::prepare()
    {
        this->closeFonts();

        std::vector<UIText*> texts;
        for (UIWindow* uiWindow : this->liveWindows())
        {
            if (uiWindow == nullptr || !uiWindow->root())
            {
                continue;
            }
            this->collectTexts(uiWindow->root().get(), texts);
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
        for (UIWindow* uiWindow : this->liveWindows())
        {
            if (uiWindow == nullptr)
            {
                continue;
            }
            if (UIPrimitive::Ref primitive = uiWindow->primitive())
            {
                primitive->setPendingUpload(font->atlasRgba(), font->atlasSize());
            }
        }
    }

    void UIHost::publishAtlas()
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
        std::vector<UIWindow*> uiWindows = this->liveWindows();
        for (UIFont* font : dirty)
        {
            for (UIWindow* uiWindow : uiWindows)
            {
                if (uiWindow == nullptr)
                {
                    continue;
                }
                if (UIPrimitive::Ref primitive = uiWindow->primitive())
                {
                    primitive->setPendingUpload(font->atlasRgba(), font->atlasSize());
                }
            }
        }
    }

    void UIHost::layout(UIWindow& window, float width, float height)
    {
        for (auto& slot : mFloating)
        {
            if (!slot.uiWindow || slot.uiWindow.get() != &window)
            {
                continue;
            }
            auto page = std::dynamic_pointer_cast<UIDockPage>(slot.uiWindow->root());
            if (!page)
            {
                continue;
            }
            page->layoutInWindow(width, height);
            return;
        }
    }

    void UIHost::flushClosing()
    {
        auto closing = std::move(mClosing);
        mClosing.clear();
        for (auto& uiWindow : closing)
        {
            if (uiWindow)
            {
                uiWindow->quit();
            }
        }
    }

    void UIHost::registerSpace(UIDockSpace* space)
    {
        if (!space) return;
        for (auto* item : mSpaces) if (item == space) return;
        space->onDestroy = [this](UIDockSpace& closing) { this->unregisterSpace(&closing); };
        mSpaces.push_back(space);
    }

    void UIHost::unregisterSpace(UIDockSpace* space)
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

    void UIHost::observe(const std::shared_ptr<UIDockPage>& page)
    {
        if (!page) return;
        page->onDrag = [this](UIDockPage& target, float x, float y, int button)
        {
            return this->dragPage(&target, x, y, button);
        };
        page->onDragEnd = [this](UIDockPage& target) { this->releasePage(&target); };
    }

    void UIHost::closeAll()
    {
        mDragging = false;
        mDragPage = nullptr;
        mSourceSpace = nullptr;
        mPreviewSpace = nullptr;
        mHostDrag = false;
        mReleasing = false;
        for (auto& slot : mFloating) this->destroySlot(slot, false);
        mFloating.clear();
    }

    std::vector<UIWindow*> UIHost::liveWindows()
    {
        std::vector<UIWindow*> uiWindows;
        for (auto& slot : mWindows)
        {
            if (slot.uiWindow) uiWindows.push_back(slot.uiWindow.get());
        }
        for (auto& slot : mFloating)
        {
            if (slot.uiWindow) uiWindows.push_back(slot.uiWindow.get());
        }
        return uiWindows;
    }

    UIHost::Slot* UIHost::slotOf(UIDockPage* page)
    {
        if (!page) return nullptr;
        for (auto& slot : mFloating)
        {
            if (slot.uiWindow && slot.uiWindow->root().get() == page) return &slot;
        }
        return nullptr;
    }

    void UIHost::destroySlot(Slot& slot, bool deferWindow)
    {
        UIWindow* window = slot.uiWindow.get();
        if (window && onDestroyWindow) onDestroyWindow(*window);
        if (slot.uiWindow)
        {
            slot.uiWindow->setRoot(nullptr);
            if (deferWindow)
            {
                mClosing.push_back(std::move(slot.uiWindow));
            }
            else
            {
                slot.uiWindow->quit();
            }
        }
    }

    void UIHost::closeFonts()
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

    void UIHost::clientToScreen(UIDockPage* page, float x, float y, float& screenX, float& screenY)
    {
        if (mHostDrag && mSourceSpace)
        {
            Rect screen = mSourceSpace->clientToScreen(x, y);
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

    void UIHost::beginFloat(UIDockPage* page, float screenX, float screenY)
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
            Rect screen;
            if (space->pageBounds(page, screen)) window = screen;
            held = space->takePage(page);
        }
        if (!held) return;
        auto uiWindow = std::make_unique<UIWindow>();
        uint32_t width = (uint32_t)window.size.x;
        uint32_t height = (uint32_t)window.size.y;
        if (width < 1) width = 1;
        if (height < 1) height = 1;
        uiWindow->init(width, height);
        held->layoutInWindow((float)width, (float)height);
        uiWindow->setRoot(held);
        mGrabScreenX = screenX - window.origin.x;
        mGrabScreenY = screenY - window.origin.y;
        Slot created;
        created.screenRect = window;
        created.uiWindow = std::move(uiWindow);
        mFloating.push_back(std::move(created));
        UIWindow& createdWindow = *mFloating.back().uiWindow;
        this->prepare();
        if (onCreateWindow) onCreateWindow(createdWindow, window);
    }

    void UIHost::updateFloat(UIDockPage* page, float screenX, float screenY)
    {
        Slot* slot = this->slotOf(page);
        if (!slot) return;
        slot->screenRect.origin.x = screenX - mGrabScreenX;
        slot->screenRect.origin.y = screenY - mGrabScreenY;
        if (slot->screenRect.size.x < 1.0f) slot->screenRect.size.x = page->shape.size.x;
        if (slot->screenRect.size.y < 1.0f) slot->screenRect.size.y = page->shape.size.y;
        if (slot->uiWindow && onPlaceWindow) onPlaceWindow(*slot->uiWindow, slot->screenRect);
        UIDockSpace* hit = nullptr;
        for (auto* space : mSpaces)
        {
            if (!space) continue;
            Rect screen = space->screenBounds();
            if (!screen.contains(Vector2(screenX, screenY)))
            {
                space->clearPreview();
                continue;
            }
            if (space->showPreview(screenX, screenY)) hit = space;
            else space->clearPreview();
        }
        if (mPreviewSpace && mPreviewSpace != hit) mPreviewSpace->clearPreview();
        mPreviewSpace = hit;
    }

    bool UIHost::dragPage(UIDockPage* page, float x, float y, int button)
    {
        if (mReleasing || button != 0 || !page) return false;
        if (!mDragging || mDragPage != page)
        {
            mDragging = true;
            mDragPage = page;
            mDragMoved = false;
            mSourceSpace = page->space();
            mHostDrag = mSourceSpace != nullptr;
            mDragSlop = mSourceSpace ? mSourceSpace->dragSlop : 4.0f;
            this->clientToScreen(page, x, y, mPressScreenX, mPressScreenY);
        }
        float screenX = 0.0f;
        float screenY = 0.0f;
        this->clientToScreen(page, x, y, screenX, screenY);
        float dx = screenX - mPressScreenX;
        float dy = screenY - mPressScreenY;
        if (!mDragMoved && (dx * dx + dy * dy) < mDragSlop * mDragSlop) return false;
        if (!mDragMoved) this->beginFloat(page, screenX, screenY);
        mDragMoved = true;
        this->updateFloat(page, screenX, screenY);
        return true;
    }

    void UIHost::releasePage(UIDockPage* page)
    {
        if (mDragPage != page || mReleasing) return;
        ReleaseGuard releasing(mReleasing);
        mDragging = false;
        mDragPage = nullptr;
        mSourceSpace = nullptr;
        mHostDrag = false;
        Slot* slot = this->slotOf(page);
        if (mDragMoved && mPreviewSpace && slot)
        {
            std::shared_ptr<UIDockPage> held;
            if (slot->uiWindow)
            {
                held = std::dynamic_pointer_cast<UIDockPage>(slot->uiWindow->root());
            }
            std::unique_ptr<UIWindow> uiWindow = std::move(slot->uiWindow);
            UIWindow* closingWindow = uiWindow.get();
            UIDockSpace* space = mPreviewSpace;
            mPreviewSpace = nullptr;
            for (auto it = mFloating.begin(); it != mFloating.end(); ++it)
            {
                if (&*it != slot) continue;
                mFloating.erase(it);
                break;
            }
            space->acceptDrop(held);
            if (uiWindow)
            {
                uiWindow->setRoot(nullptr);
                mClosing.push_back(std::move(uiWindow));
            }
            this->prepare();
            if (closingWindow && onDestroyWindow) onDestroyWindow(*closingWindow);
        }
        else if (mPreviewSpace)
        {
            mPreviewSpace->clearPreview();
            mPreviewSpace = nullptr;
        }
        mDragMoved = false;
    }

    void UIHost::collectTexts(UIWidget* widget, std::vector<UIText*>& texts)
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

        for (auto& child : widget->children())
        {
            this->collectTexts(child.get(), texts);
        }
    }

    String UIHost::resolveAssetPath(const char* relativePath) const
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

    UIFont* UIHost::loadFont(const char* fontPath, uint32_t pixelSize)
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
