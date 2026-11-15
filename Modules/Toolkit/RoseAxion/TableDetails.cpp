#include "./TableDetails.h"
#include "./FieldTypes.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"
#include "UI/UIStroke.h"

#include <cmath>
#include <utility>
#include <vector>

namespace eokas::RoseAxion {

    namespace {

        struct TypeDrop : UIWidget
        {
            Color field;
            Color hoverFill;
            Color pressedFill;
            UIStrokeStyle border;
            Color chevron;
            bool open = false;
            float paddingX = 8.0f;

            void layout() override
            {
                if (!visible)
                {
                    return;
                }
                if (children().empty() || !children()[0])
                {
                    return;
                }
                UIWidget& label = *children()[0];
                label.layout();
                float y = (shape.size.y - label.shape.size.y) * 0.5f;
                if (y < 0.0f)
                {
                    y = 0.0f;
                }
                this->placeChild(label, Vector2(floorf(paddingX + 0.5f), floorf(y + 0.5f)));
            }

            void render(UIPrimitive& primitive) override
            {
                if (!visible)
                {
                    return;
                }
                Matrix3 localToScreen = this->matrixLocalToScreen();
                Color bg = field;
                if (this->isPressed())
                {
                    bg = pressedFill;
                }
                else if (this->isHovered() || open)
                {
                    bg = hoverFill;
                }
                primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), bg);
                UIStroke::border(primitive, localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), border);
                float cs = shape.size.y * 0.28f;
                if (cs > 8.0f)
                {
                    cs = 8.0f;
                }
                if (cs < 4.0f)
                {
                    cs = 4.0f;
                }
                float cx = shape.size.x - paddingX - cs * 0.5f;
                float cy = shape.size.y * 0.5f;
                Rect uv = UIFont::solidUV();
                if (open)
                {
                    primitive.addQuad(
                        this->localToScreen(Vector2(cx - cs * 0.5f, cy + cs * 0.25f)),
                        this->localToScreen(Vector2(cx + cs * 0.5f, cy + cs * 0.25f)),
                        this->localToScreen(Vector2(cx, cy - cs * 0.35f)),
                        this->localToScreen(Vector2(cx, cy - cs * 0.35f)),
                        uv,
                        chevron);
                }
                else
                {
                    primitive.addQuad(
                        this->localToScreen(Vector2(cx - cs * 0.5f, cy - cs * 0.25f)),
                        this->localToScreen(Vector2(cx + cs * 0.5f, cy - cs * 0.25f)),
                        this->localToScreen(Vector2(cx, cy + cs * 0.35f)),
                        this->localToScreen(Vector2(cx, cy + cs * 0.35f)),
                        uv,
                        chevron);
                }
                UIWidget::render(primitive);
            }
        };

        struct SettingsPanel : UIWidget
        {
            UIWidget* footer = nullptr;
            float pad = 16.0f;
            void layout() override
            {
                if (!visible)
                {
                    return;
                }
                for (auto& child : children())
                {
                    if (!child || child.get() == footer)
                    {
                        continue;
                    }
                    child->layout();
                }
                if (footer == nullptr)
                {
                    return;
                }
                footer->layout();
                float y = shape.size.y - pad - footer->shape.size.y;
                if (y < pad)
                {
                    y = pad;
                }
                this->placeChild(*footer, Vector2(floorf(pad + 0.5f), floorf(y + 0.5f)));
            }
        };

    }

    void RoseTableDetails::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
        mOverlay = host.overlay;
    }

    void RoseTableDetails::setOverlay(UIWidget* overlay)
    {
        if (mOverlay == overlay)
        {
            return;
        }
        this->captureObjectState();
        if (mObjectPanel != nullptr)
        {
            mObjectPanel->clearOverlayEditors();
        }
        this->clearTypeMenu();
        mOverlay = overlay;
        mHost.overlay = overlay;
    }

    bool RoseTableDetails::hasPopupEdit() const
    {
        if (mTypeOpen)
        {
            return true;
        }
        return mObjectPanel != nullptr && mObjectPanel->hasPopupEdit();
    }

    void RoseTableDetails::clearTypeMenu()
    {
        if (mOverlay != nullptr && mTypeMenu != nullptr)
        {
            mOverlay->detachChild(mTypeMenu);
        }
        mTypeMenu = nullptr;
        mTypeAnchor = nullptr;
    }

    void RoseTableDetails::applyFieldType(int column, const String& typeName)
    {
        mTypeOpen = false;
        if (mTypeMenu != nullptr)
        {
            mTypeMenu->visible = false;
        }
        if (mDoc == nullptr || column < 0)
        {
            this->notifyRebuild();
            return;
        }
        const u32_t index = static_cast<u32_t>(column);
        if (typeName == mDoc->column(index).schemaName)
        {
            this->notifyRebuild();
            return;
        }
        if (mDoc->columnChangeClears(index, typeName))
        {
            if (mHost.confirmChangeType)
            {
                mHost.confirmChangeType(column, typeName);
            }
        }
        else
        {
            mDoc->setColumnType(index, typeName);
        }
        this->notifyRebuild();
    }

    const String& RoseTableDetails::font() const
    {
        return mHost.font;
    }

    float RoseTableDetails::scale() const
    {
        return hostScale(mHost.scale);
    }

    RoseObjectEditor::Host RoseTableDetails::objectHost()
    {
        RoseObjectEditor::Host host;
        host.requestRebuild = [this]() { this->notifyRebuild(); };
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.openExternal = mHost.openExternal;
        host.pickFilePath = mHost.pickFilePath;
        host.pickFolderPath = mHost.pickFolderPath;
        host.ctrlDown = mHost.ctrlDown;
        host.overlay = mOverlay;
        host.font = mHost.font;
        host.scale = mHost.scale;
        return host;
    }

    void RoseTableDetails::captureObjectState()
    {
        if (mObjectPanel == nullptr)
        {
            return;
        }
        mObjectScrollY = mObjectPanel->scrollY();
        mObjectCollapsed = mObjectPanel->collapsed();
        mObjectEdit = mObjectPanel->valueEdit();
        if (mObjectPanel->row() < 0)
        {
            this->clearObject();
        }
    }

    void RoseTableDetails::clearObject()
    {
        mObjectRow = -1;
        mObjectCol = -1;
        mObjectTable = -1;
        mObjectScrollY = 0.0f;
        mObjectCollapsed.clear();
        mObjectEdit = {};
    }

    void RoseTableDetails::notifyRebuild()
    {
        this->captureObjectState();
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    void RoseTableDetails::clampState()
    {
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveTable())
        {
            mSettingsCol = -1;
            this->clearObject();
            return;
        }
        int rows = static_cast<int>(mDoc->rowCount());
        int cols = static_cast<int>(mDoc->columnCount());
        if (mSettingsCol >= cols)
        {
            mSettingsCol = -1;
        }
        if (mSettingsCol < 0)
        {
            mTypeOpen = false;
        }
        if (mObjectRow >= 0)
        {
            bool sameTable = static_cast<int>(mDoc->activeTableIndex()) == mObjectTable;
            if (!sameTable || mObjectRow >= rows)
            {
                this->clearObject();
            }
            else if (mObjectCol >= 0 && (mObjectCol >= cols || !mDoc->isNestedStructColumn(static_cast<u32_t>(mObjectCol))))
            {
                this->clearObject();
            }
        }
    }

    void RoseTableDetails::reset()
    {
        mSettingsCol = -1;
        mTypeOpen = false;
        this->clearObject();
        this->setOverlay(nullptr);
        mSettingsPanel = nullptr;
        mObjectPanel = nullptr;
        mHint = nullptr;
        mHeader = nullptr;
        mFocus = nullptr;
        this->detachChildren();
    }

    void RoseTableDetails::close()
    {
        mSettingsCol = -1;
        mTypeOpen = false;
        this->clearTypeMenu();
        this->clearObject();
    }

    void RoseTableDetails::openObject(int row, int col)
    {
        if (mDoc == nullptr || !mDoc->isOpen() || row < 0)
        {
            return;
        }
        if (mObjectRow != row || mObjectCol != col)
        {
            mObjectScrollY = 0.0f;
            mObjectCollapsed.clear();
            mObjectEdit = {};
        }
        mObjectRow = row;
        mObjectCol = col;
        mObjectTable = static_cast<int>(mDoc->activeTableIndex());
        mSettingsCol = -1;
        mTypeOpen = false;
        this->clearTypeMenu();
    }

    void RoseTableDetails::openField(int col)
    {
        mSettingsCol = col;
        mTypeOpen = false;
        this->clearTypeMenu();
        this->clearObject();
    }

    void RoseTableDetails::selectField(int col)
    {
        if (mSettingsCol < 0 || mSettingsCol == col)
        {
            return;
        }
        this->openField(col);
    }

    void RoseTableDetails::noteRowDeleted(int row)
    {
        if (mObjectRow == row)
        {
            this->clearObject();
        }
        else if (mObjectRow > row)
        {
            --mObjectRow;
        }
    }

    void RoseTableDetails::noteRowInserted(int index)
    {
        if (mObjectRow >= index)
        {
            ++mObjectRow;
        }
    }

    bool RoseTableDetails::handleEscape()
    {
        if (mObjectPanel != nullptr && mObjectPanel->handleEscape())
        {
            return true;
        }
        if (mTypeOpen)
        {
            mTypeOpen = false;
            this->clearTypeMenu();
            this->notifyRebuild();
            return true;
        }
        if (mSettingsCol >= 0 || mObjectRow >= 0)
        {
            this->close();
            this->notifyRebuild();
            return true;
        }
        return false;
    }

    bool RoseTableDetails::colorDropping() const
    {
        return mObjectPanel != nullptr && mObjectPanel->colorDropping();
    }

    void RoseTableDetails::sampleColorDropper(float x, float y, bool commit)
    {
        if (mObjectPanel != nullptr)
        {
            mObjectPanel->sampleColorDropper(x, y, commit);
        }
    }

    void RoseTableDetails::cancelColorDropper()
    {
        if (mObjectPanel != nullptr)
        {
            mObjectPanel->cancelColorDropper();
        }
    }

    void RoseTableDetails::layout()
    {
        if (!visible)
        {
            return;
        }
        const float width = shape.size.x;
        const float height = shape.size.y;
        const float headerH = mHeader != nullptr ? 36.0f * this->scale() : 0.0f;
        if (mHeader != nullptr)
        {
            setBox(*mHeader, Rect(0.0f, 0.0f, width, headerH));
        }
        if (mObjectPanel != nullptr)
        {
            setBox(*mObjectPanel, Rect(0.0f, 0.0f, width, height));
            mObjectPanel->layout();
        }
        if (mSettingsPanel != nullptr)
        {
            setBox(*mSettingsPanel, Rect(0.0f, 0.0f, width, height));
            mSettingsPanel->layout();
        }
        if (mOverlay != nullptr && mTypeMenu != nullptr && mTypeAnchor != nullptr)
        {
            placePopup(*mOverlay, *mTypeMenu, mTypeAnchor->screenBounds());
        }
        if (mHint != nullptr)
        {
            mHint->layout();
            float bodyH = height - headerH;
            if (bodyH < 1.0f)
            {
                bodyH = 1.0f;
            }
            setBox(*mHint, Rect((width - mHint->shape.size.x) * 0.5f, headerH + (bodyH - mHint->shape.size.y) * 0.5f, mHint->shape.size.x, mHint->shape.size.y));
        }
        UIWidget::layout();
    }

    void RoseTableDetails::rebuild()
    {
        this->captureObjectState();
        this->clearTypeMenu();
        this->detachChildren();
        mSettingsPanel = nullptr;
        mObjectPanel = nullptr;
        mHint = nullptr;
        mHeader = nullptr;
        mFocus = nullptr;
        mTypeAnchor = nullptr;
        this->clampState();
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }

        const float s = this->scale();
        const float fontPx = 15.0f * s;
        const float smallPx = 13.0f * s;
        const RoseTheme& t = theme();
        const Color ink = t.ink;
        const Color mute = t.mute;
        const Color quiet = t.quiet;
        const Color field = t.field;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const String& fontPath = this->font();
        const float panelW = shape.size.x > 1.0f ? shape.size.x : 360.0f * s;
        const float panelH = shape.size.y > 1.0f ? shape.size.y : 400.0f * s;
        fillSurface(*this, t.canvas);
        this->pickable = true;

        if (mSettingsCol >= 0)
        {
            TableColumn info = mDoc->column(static_cast<u32_t>(mSettingsCol));
            auto panel = std::make_shared<SettingsPanel>();
            panel->pickable = true;
            panel->color = t.raised;
            panel->shape.pivot = Vector2(0.0f, 0.0f);
            panel->pad = 16.0f * s;
            setBox(panel, Rect(0.0f, 0.0f, panelW, panelH));
            float innerW = panelW - 32.0f * s;
            if (innerW < 80.0f * s)
            {
                innerW = 80.0f * s;
            }
            auto body = std::make_shared<UIList>();
            body->direction = UIDirection::Vertical;
            body->padding = 16.0f * s;
            body->spacing = 10.0f * s;
            body->color = clear;
            body->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(body, Rect(0.0f, 0.0f, panelW, 0.0f));
            auto head = std::make_shared<UIWidget>();
            head->pickable = false;
            head->shape.pivot = Vector2(0.0f, 0.0f);
            head->color = clear;
            setBox(head, Rect(0.0f, 0.0f, innerW, 32.0f * s));
            auto title = makeText("字段设置", fontPath, fontPx + 1.0f, ink, innerW - 76.0f * s, 28.0f * s);
            title->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(title, Rect(0.0f, 2.0f * s, innerW - 76.0f * s, 28.0f * s));
            head->attachChild(title);
            auto close = makeButton("关闭", fontPath, smallPx, 64.0f * s, 28.0f * s, quiet, ink);
            close->shape.setBox(Vector2(innerW - 64.0f * s, 2.0f * s), Vector2(64.0f * s, 28.0f * s));
            close->onClick = [this]()
            {
                this->close();
                this->notifyRebuild();
            };
            head->attachChild(close);
            body->attachChild(head);
            body->attachChild(makeText("字段名称", fontPath, smallPx, mute, innerW, 18.0f * s));
            auto name = std::make_shared<UIInput>();
            name->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(name, Rect(0.0f, 0.0f, innerW, 32.0f * s));
            name->paddingX = 6.0f * s;
            name->paddingY = 4.0f * s;
            name->background = field;
            name->hoverFill = hoverOf(field);
            name->border.color = t.ring;
            name->border.thickness = 1.0f;
            name->caret.color = ink;
            name->selection = t.selection;
            if (UIText* label = name->label())
            {
                styleText(*label, fontPath, fontPx, ink);
            }
            name->writeClipboard = mHost.writeClipboard;
            name->readClipboard = mHost.readClipboard;
            name->setText(info.name);
            int settingsCol = mSettingsCol;
            UIInput* nameRaw = name.get();
            name->onFocusLoss = [this, nameRaw, settingsCol]()
            {
                if (nameRaw == nullptr)
                {
                    return;
                }
                if (mDoc->column(static_cast<u32_t>(settingsCol)).name == nameRaw->text)
                {
                    return;
                }
                mDoc->setColumnName(static_cast<u32_t>(settingsCol), nameRaw->text);
                this->notifyRebuild();
            };
            name->onSubmit = [this, settingsCol](const String& text)
            {
                mDoc->setColumnName(static_cast<u32_t>(settingsCol), text);
                this->notifyRebuild();
            };
            body->attachChild(name);
            body->attachChild(makeText("字段类型", fontPath, smallPx, mute, innerW, 18.0f * s));
            std::vector<String> typeNames = mDoc->columnTypeNames();
            auto drop = std::make_shared<TypeDrop>();
            drop->shape.pivot = Vector2(0.0f, 0.0f);
            drop->field = field;
            drop->hoverFill = hoverOf(field);
            drop->pressedFill = pressOf(field);
            drop->border.color = t.ring;
            drop->border.thickness = 1.0f;
            drop->chevron = mute;
            drop->paddingX = 8.0f * s;
            drop->open = mTypeOpen;
            setBox(drop, Rect(0.0f, 0.0f, innerW, 32.0f * s));
            auto typeLabel = makeText(schemaLabel(info.schemaName), fontPath, fontPx, ink, innerW - 28.0f * s, 20.0f * s);
            typeLabel->pickable = false;
            drop->attachChild(typeLabel);
            TypeDrop* dropRaw = drop.get();
            auto showTypeMenu = [this, s, fontPx, dropRaw, settingsCol, typeNames, fontPath]()
            {
                if (mOverlay == nullptr || dropRaw == nullptr)
                {
                    return;
                }
                this->clearTypeMenu();
                auto menu = std::make_shared<UIMenu>();
                menu->direction = UIDirection::Vertical;
                menu->padding = 2.0f * s;
                menu->spacing = 1.0f * s;
                menu->color = theme().popup;
                menu->overlay = true;
                menu->shape.pivot = Vector2(0.0f, 0.0f);
                menu->host = dropRaw;
                std::vector<UIWidget*> itemWidgets;
                std::vector<String> itemTypes;
                itemWidgets.reserve(typeNames.size());
                itemTypes.reserve(typeNames.size());
                for (const String& typeName : typeNames)
                {
                    auto item = std::make_shared<UIMenuItem>();
                    item->paddingX = 10.0f * s;
                    item->paddingY = 3.0f * s;
                    item->setText(schemaLabel(typeName));
                    styleMenuItem(*item, fontPath, menuFontPx(s));
                    UIWidget* itemRaw = item.get();
                    item->onClick = [this, settingsCol, typeName]()
                    {
                        this->applyFieldType(settingsCol, typeName);
                    };
                    item->onPointerPress = [this, settingsCol, typeName](const UIPointerEvent&)
                    {
                        this->applyFieldType(settingsCol, typeName);
                    };
                    menu->addItem(item);
                    itemWidgets.push_back(itemRaw);
                    itemTypes.push_back(typeName);
                }
                menu->onPointerPressAnywhere = [this, dropRaw, settingsCol, itemWidgets, itemTypes](const UIPointerEvent& event)
                {
                    for (size_t i = 0; i < itemWidgets.size() && i < itemTypes.size(); ++i)
                    {
                        UIWidget* item = itemWidgets[i];
                        if (item == nullptr)
                        {
                            continue;
                        }
                        if (item->screenBounds().contains(event.position))
                        {
                            this->applyFieldType(settingsCol, itemTypes[i]);
                            return;
                        }
                    }
                    if (dropRaw != nullptr && dropRaw->screenBounds().contains(event.position))
                    {
                        return;
                    }
                    if (mTypeOpen)
                    {
                        mTypeOpen = false;
                        if (mTypeMenu != nullptr)
                        {
                            mTypeMenu->visible = false;
                        }
                        this->notifyRebuild();
                    }
                };
                mOverlay->attachChild(menu);
                mTypeMenu = menu.get();
                mTypeAnchor = dropRaw;
                placePopup(*mOverlay, *menu, dropRaw->screenBounds());
            };
            drop->onClick = [this, dropRaw, showTypeMenu]()
            {
                if (dropRaw == nullptr)
                {
                    return;
                }
                if (mTypeOpen)
                {
                    mTypeOpen = false;
                    this->notifyRebuild();
                    return;
                }
                mTypeOpen = true;
                dropRaw->open = true;
                showTypeMenu();
            };
            body->attachChild(drop);
            mTypeAnchor = dropRaw;
            if (mTypeOpen)
            {
                showTypeMenu();
            }
            panel->attachChild(body);

            auto removeField = makeDangerButton("删除字段", fontPath, fontPx, innerW, 32.0f * s);
            setBox(removeField, Rect(0.0f, 0.0f, innerW, 32.0f * s));
            removeField->onClick = [this, settingsCol]()
            {
                if (mHost.confirmDeleteColumn)
                {
                    mHost.confirmDeleteColumn(settingsCol);
                }
            };
            panel->attachChild(removeField);
            panel->footer = removeField.get();
            this->attachChild(panel);
            mSettingsPanel = panel.get();
            mFocus = nameRaw;
            return;
        }

        if (mObjectRow >= 0)
        {
            auto panel = std::make_shared<RoseObjectEditor>();
            panel->pickable = true;
            panel->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(panel, Rect(0.0f, 0.0f, panelW, panelH));
            panel->bind(*mDoc, this->objectHost());
            panel->setRow(mObjectRow);
            panel->setColumn(mObjectCol);
            panel->setCollapsed(mObjectCollapsed);
            panel->setValueEdit(mObjectEdit);
            this->attachChild(panel);
            panel->rebuild();
            panel->setScroll(mObjectScrollY);
            mObjectPanel = panel.get();
            mFocus = panel->focusWidget();
            return;
        }

        const float headerH = 36.0f * s;
        const float pad = 8.0f * s;
        auto header = std::make_shared<UIWidget>();
        header->shape.pivot = Vector2(0.0f, 0.0f);
        header->pickable = false;
        fillSurface(*header, t.header);
        setBox(header, Rect(0.0f, 0.0f, panelW, headerH));
        auto title = makeText("详情", fontPath, fontPx, ink, panelW - pad * 2.0f, 22.0f * s);
        title->layout();
        setBox(title, Rect(pad, (headerH - title->shape.size.y) * 0.5f, title->shape.size.x, title->shape.size.y));
        header->attachChild(title);
        auto headerRule = makeSplit(panelW, false);
        setBox(headerRule, Rect(0.0f, headerH - 1.0f, panelW, 1.0f));
        header->attachChild(headerRule);
        this->attachChild(header);
        mHeader = header.get();

        auto hint = makeText("选择一行或字段查看详情", fontPath, fontPx, mute, panelW - 32.0f * s, 28.0f * s);
        hint->layout();
        float bodyH = panelH - headerH;
        if (bodyH < 1.0f)
        {
            bodyH = 1.0f;
        }
        setBox(hint, Rect((panelW - hint->shape.size.x) * 0.5f, headerH + (bodyH - hint->shape.size.y) * 0.5f, hint->shape.size.x, hint->shape.size.y));
        this->attachChild(hint);
        mHint = hint.get();
    }

}
