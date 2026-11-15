#include "./TableOutliner.h"
#include "./FieldEditors.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"

#include <cmath>
#include <utility>
#include <vector>

namespace eokas::RoseAxion {

    void RoseTableOutliner::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseTableOutliner::setOverlay(UIWidget* overlay)
    {
        if (mOverlay == overlay)
        {
            return;
        }
        if (mOverlay != nullptr)
        {
            if (mRenameInput != nullptr)
            {
                mOverlay->detachChild(mRenameInput);
            }
            if (mMenu != nullptr)
            {
                mOverlay->detachChild(mMenu);
            }
        }
        mRenameInput = nullptr;
        mMenu = nullptr;
        mOverlay = overlay;
    }

    const String& RoseTableOutliner::font() const
    {
        return mHost.font;
    }

    float RoseTableOutliner::scale() const
    {
        return hostScale(mHost.scale);
    }

    void RoseTableOutliner::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    void RoseTableOutliner::reset()
    {
        mRenameIndex = -1;
        mRenameSeed = String::empty;
        mKeepY = 0.0f;
        mView = nullptr;
        mOverlay = nullptr;
        mRenameAnchor = nullptr;
        mRenameInput = nullptr;
        mMenu = nullptr;
        mFocus = nullptr;
        this->detachChildren();
    }

    float RoseTableOutliner::scrollY() const
    {
        return mView != nullptr ? mView->scrollY() : mKeepY;
    }

    void RoseTableOutliner::setScroll(float y)
    {
        mKeepY = y;
        if (mView != nullptr)
        {
            mView->setScroll(0.0f, y);
        }
    }

    void RoseTableOutliner::layout()
    {
        this->placeOverlayEditors();
        UIWidget::layout();
    }

    void RoseTableOutliner::placeOverlayEditors()
    {
        if (mOverlay == nullptr)
        {
            return;
        }
        if (mRenameInput != nullptr && mRenameAnchor != nullptr)
        {
            placeOnOverlay(*mOverlay, *mRenameInput, mRenameAnchor->screenBounds());
        }
        if (mMenu != nullptr && mMenu->visible && mMenu->parent() == mOverlay)
        {
            UIMenu* menu = dynamic_cast<UIMenu*>(mMenu);
            if (menu != nullptr && menu->host != nullptr)
            {
                placePopup(*mOverlay, *menu, menu->host->screenBounds());
            }
        }
    }

    void RoseTableOutliner::openTable(u32_t index)
    {
        if (mDoc == nullptr)
        {
            return;
        }
        this->cancelRename();
        if (mDoc->selectTable(index))
        {
            this->notifyRebuild();
        }
    }

    void RoseTableOutliner::beginRename(u32_t index)
    {
        if (mDoc == nullptr || index >= mDoc->tableCount())
        {
            return;
        }
        mRenameIndex = static_cast<int>(index);
        mRenameSeed = mDoc->tableName(index);
        this->notifyRebuild();
    }

    void RoseTableOutliner::finishRename(const String& text)
    {
        if (mDoc == nullptr || mRenameIndex < 0)
        {
            return;
        }
        int index = mRenameIndex;
        mRenameInput = nullptr;
        mRenameIndex = -1;
        if (!mDoc->renameTable(static_cast<u32_t>(index), text))
        {
            mRenameSeed = text;
            mRenameIndex = index;
            this->notifyRebuild();
            return;
        }
        this->notifyRebuild();
    }

    void RoseTableOutliner::cancelRename()
    {
        if (mRenameIndex < 0)
        {
            return;
        }
        mRenameIndex = -1;
        this->notifyRebuild();
    }

    bool RoseTableOutliner::handleEscape()
    {
        if (mMenu != nullptr && mMenu->visible)
        {
            mMenu->visible = false;
            return true;
        }
        if (mRenameIndex >= 0)
        {
            this->cancelRename();
            return true;
        }
        return false;
    }

    void RoseTableOutliner::rebuild()
    {
        if (mOverlay != nullptr)
        {
            if (mRenameInput != nullptr)
            {
                mOverlay->detachChild(mRenameInput);
            }
            if (mMenu != nullptr)
            {
                mOverlay->detachChild(mMenu);
            }
        }
        mRenameInput = nullptr;
        mMenu = nullptr;
        this->detachChildren();
        mView = nullptr;
        mRenameAnchor = nullptr;
        mFocus = nullptr;
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }

        const float s = this->scale();
        const float fontPx = 15.0f * s;
        const float smallPx = 13.0f * s;
        const float headerH = 36.0f * s;
        const float itemH = 32.0f * s;
        const float pad = 8.0f * s;
        const RoseTheme& t = theme();
        const Color canvas = t.canvas;
        const Color headerBg = t.header;
        const Color selected = t.selected;
        const Color ink = t.ink;
        const Color accent = t.accent;
        const Color popup = t.popup;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const Color line = t.line;
        const String& fontPath = this->font();
        const float width = shape.size.x;
        const float height = shape.size.y;
        fillSurface(*this, canvas);
        this->shape.pivot = Vector2(0.0f, 0.0f);

        auto header = std::make_shared<UIWidget>();
        header->shape.pivot = Vector2(0.0f, 0.0f);
        header->pickable = false;
        header->color = headerBg;
        setBox(header, Rect(0.0f, 0.0f, width, headerH));
        auto title = makeText("表格", fontPath, fontPx, ink, width - 48.0f * s, 22.0f * s);
        title->layout();
        setBox(title, Rect(pad, (headerH - title->shape.size.y) * 0.5f, title->shape.size.x, title->shape.size.y));
        header->attachChild(title);
        const float addW = 24.0f * s;
        const float addH = 24.0f * s;
        auto add = makeButton("+", fontPath, fontPx, addW, addH, accent, t.inkOnAccent);
        add->paddingX = 0.0f;
        add->paddingY = 0.0f;
        setBox(add, Rect(width - addW - pad, (headerH - addH) * 0.5f, addW, addH));
        add->onClick = [this]()
        {
            if (mDoc == nullptr)
            {
                return;
            }
            this->cancelRename();
            if (mDoc->addTable())
            {
                this->notifyRebuild();
            }
        };
        header->attachChild(add);
        auto headerRule = std::make_shared<UIWidget>();
        headerRule->pickable = false;
        headerRule->color = line;
        headerRule->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(headerRule, Rect(0.0f, headerH - 1.0f, width, 1.0f));
        header->attachChild(headerRule);
        this->attachChild(header);

        auto showPopup = [this, s, fontPx, popup, fontPath](UIWidget& anchor, const std::vector<std::pair<const char*, std::function<void()>>>& entries)
        {
            if (mOverlay == nullptr)
            {
                return;
            }
            auto menu = std::make_shared<UIMenu>();
            menu->direction = UIDirection::Vertical;
            menu->padding = 4.0f * s;
            menu->spacing = 2.0f * s;
            menu->color = popup;
            menu->overlay = true;
            menu->shape.pivot = Vector2(0.0f, 0.0f);
            menu->host = &anchor;
            for (const auto& entry : entries)
            {
                auto item = std::make_shared<UIMenuItem>();
                item->paddingX = 12.0f * s;
                item->paddingY = 8.0f * s;
                item->setText(entry.first);
                styleMenuItem(*item, fontPath, fontPx);
                auto action = entry.second;
                UIMenu* menuRaw = menu.get();
                item->onClick = [this, menuRaw, action]()
                {
                    if (menuRaw != nullptr)
                    {
                        menuRaw->visible = false;
                    }
                    mMenu = nullptr;
                    action();
                };
                menu->addItem(item);
            }
            mOverlay->attachChild(menu);
            mMenu = menu.get();
            placePopup(*mOverlay, *menu, anchor.screenBounds());
        };

        float listY = headerH;
        float listH = height - headerH;
        if (listH < 1.0f)
        {
            listH = 1.0f;
        }
        auto view = std::make_shared<UIView>();
        view->shape.pivot = Vector2(0.0f, 0.0f);
        fillSurface(*view, canvas);
        setBox(view, Rect(0.0f, listY, width, listH));

        auto list = std::make_shared<UIList>();
        list->direction = UIDirection::Vertical;
        list->padding = 4.0f * s;
        list->spacing = 2.0f * s;
        list->color = clear;
        list->shape.pivot = Vector2(0.0f, 0.0f);

        const u32_t count = mDoc->tableCount();
        const bool hasActive = mDoc->hasActiveTable();
        const u32_t active = mDoc->activeTableIndex();
        const float moreW = 28.0f * s;
        const float moreH = 24.0f * s;
        const float moreGap = 4.0f * s;
        for (u32_t index = 0; index < count; ++index)
        {
            const bool isActive = hasActive && active == index;
            const bool renaming = mRenameIndex == static_cast<int>(index);
            auto item = std::make_shared<UIWidget>();
            item->shape.pivot = Vector2(0.0f, 0.0f);
            item->color = isActive ? selected : clear;
            setBox(item, Rect(0.0f, 0.0f, width - 8.0f * s, itemH));
            item->onClick = [this, index]() { this->openTable(index); };
            if (isActive)
            {
                auto mark = std::make_shared<UIWidget>();
                mark->pickable = false;
                mark->color = accent;
                setBox(mark, Rect(0.0f, 6.0f * s, 3.0f * s, itemH - 12.0f * s));
                item->attachChild(mark);
            }

            const float nameW = item->shape.size.x - pad * 2.0f - moreW - moreGap;
            auto nameText = makeText(mDoc->tableName(index), fontPath, smallPx, ink, nameW > 0.0f ? nameW : 1.0f, 20.0f * s);
            nameText->layout();
            float nameH = nameText->shape.size.y;
            if (nameH < 1.0f)
            {
                nameH = 20.0f * s;
            }
            setBox(nameText, Rect(pad, (itemH - nameH) * 0.5f, nameW > 0.0f ? nameW : 1.0f, nameH));
            item->attachChild(nameText);

            if (renaming)
            {
                mRenameAnchor = item.get();
            }
            else
            {
                auto more = makeMoreButton(fontPath, fontPx, moreW, moreH, ink);
                more->visible = false;
                setBox(more, Rect(item->shape.size.x - moreW - moreGap, (itemH - moreH) * 0.5f, moreW, moreH));
                UIWidget* itemRaw = item.get();
                UIButton* moreRaw = more.get();
                auto showMore = [moreRaw](const UIPointerEvent&)
                {
                    if (moreRaw != nullptr)
                    {
                        moreRaw->visible = true;
                    }
                };
                auto hideMore = [itemRaw, moreRaw, this](const UIPointerEvent& event)
                {
                    if (mMenu != nullptr && mMenu->visible)
                    {
                        return;
                    }
                    if (itemRaw == nullptr || moreRaw == nullptr)
                    {
                        return;
                    }
                    Vector2 local = itemRaw->screenToLocal(event.position);
                    if (!itemRaw->contains(local))
                    {
                        moreRaw->visible = false;
                    }
                };
                item->onPointerEnter = showMore;
                item->onPointerLeave = hideMore;
                more->onPointerEnter = showMore;
                more->onPointerLeave = hideMore;
                more->onClick = [this, index, moreRaw, showPopup]()
                {
                    showPopup(*moreRaw, {
                        {"打开", [this, index]() { this->openTable(index); }},
                        {"重命名", [this, index]() { this->beginRename(index); }},
                        {"删除", [this, index]()
                        {
                            if (mHost.confirmDeleteTable)
                            {
                                mHost.confirmDeleteTable(static_cast<int>(index));
                            }
                        }},
                    });
                    if (moreRaw != nullptr)
                    {
                        moreRaw->visible = true;
                    }
                };
                item->attachChild(more);
            }
            list->attachChild(item);
        }
        list->layout();
        view->addChild(list);
        view->setScroll(0.0f, mKeepY);
        this->attachChild(view);
        mView = view.get();

        if (mRenameIndex >= 0 && mRenameAnchor != nullptr && mOverlay != nullptr)
        {
            auto input = std::make_shared<EscapeInput>();
            input->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(input, Rect(0.0f, 0.0f, width - 16.0f * s, itemH));
            input->paddingX = pad;
            input->paddingY = 4.0f * s;
            input->background = t.paper;
            input->hoverFill = input->background;
            input->border.color = accent;
            input->border.thickness = 1.0f;
            input->caret.color = ink;
            input->selection = t.selection;
            if (UIText* label = input->label())
            {
                styleText(*label, fontPath, smallPx, ink);
            }
            input->setText(mRenameSeed);
            UIInput* inputRaw = input.get();
            input->onSubmit = [this](const String& text) { this->finishRename(text); };
            input->onEscape = [this]() { this->handleEscape(); };
            input->onFocusLoss = [this, inputRaw]()
            {
                if (mRenameIndex >= 0)
                {
                    this->finishRename(String(inputRaw->text));
                }
            };
            mOverlay->attachChild(input);
            mRenameInput = input.get();
            mFocus = input.get();
        }
    }

}
