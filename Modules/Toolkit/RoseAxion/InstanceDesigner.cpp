#include "./InstanceDesigner.h"
#include "./InstanceKind.h"
#include "./InstanceEditor_ListGrid.h"
#include "./InstanceEditor_Struct.h"
#include "./FieldEditors.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"

namespace eokas::RoseAxion {

    namespace {

        std::vector<String> removeLabel(const std::vector<String>& current, const String& label)
        {
            std::vector<String> next;
            for (const String& item : current)
            {
                if (item != label)
                {
                    next.push_back(item);
                }
            }
            return next;
        }

    }

    void RoseInstanceDesigner::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseInstanceDesigner::setOverlay(UIWidget* overlay)
    {
        mOverlay = overlay;
        mHost.overlay = overlay;
        if (mStructEditor)
        {
            mStructEditor->setOverlay(overlay);
        }
        if (mListGridEditor)
        {
            mListGridEditor->setOverlay(overlay);
        }
    }

    void RoseInstanceDesigner::reset()
    {
        mEdit = {};
        mKeepY = 0.0f;
        mView = nullptr;
        mFocus = nullptr;
        mNameInput = nullptr;
        mHeaderBar = nullptr;
        mEditorTop = 0.0f;
        this->clearOverlayEditors();
        if (mStructEditor)
        {
            mStructEditor->reset();
        }
        if (mListGridEditor)
        {
            mListGridEditor->reset();
        }
        this->detachChildren();
    }

    const String& RoseInstanceDesigner::font() const
    {
        return mHost.font;
    }

    float RoseInstanceDesigner::scale() const
    {
        return hostScale(mHost.scale);
    }

    void RoseInstanceDesigner::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    float RoseInstanceDesigner::scrollY() const
    {
        if (mListGridEditor != nullptr && mListGridEditor->parent() == this)
        {
            return mListGridEditor->scrollY();
        }
        if (mStructEditor != nullptr && mStructEditor->parent() == this)
        {
            return mStructEditor->scrollY();
        }
        return mView != nullptr ? mView->scrollY() : mKeepY;
    }

    void RoseInstanceDesigner::setScroll(float y)
    {
        mKeepY = y;
        if (mListGridEditor != nullptr && mListGridEditor->parent() == this)
        {
            mListGridEditor->setScroll(mListGridEditor->scrollX(), y);
            return;
        }
        if (mStructEditor != nullptr && mStructEditor->parent() == this)
        {
            mStructEditor->setScroll(y);
            return;
        }
        if (mView != nullptr)
        {
            mView->setScroll(0.0f, y);
        }
    }

    void RoseInstanceDesigner::clearOverlayEditors()
    {
        if (mOverlay != nullptr)
        {
            if (mPickMenu != nullptr)
            {
                mOverlay->detachChild(mPickMenu);
            }
            if (mDatePanel != nullptr)
            {
                mOverlay->detachChild(mDatePanel);
            }
            if (mColorPanel != nullptr)
            {
                mOverlay->detachChild(mColorPanel);
            }
        }
        mPickMenu = nullptr;
        mDatePanel = nullptr;
        mColorPanel = nullptr;
        mColorPicker = nullptr;
        mPickAnchor = nullptr;
        mDateAnchor = nullptr;
        mColorAnchor = nullptr;
    }

    void RoseInstanceDesigner::layout()
    {
        if (!visible)
        {
            return;
        }
        if (mHeaderBar != nullptr)
        {
            setBox(*mHeaderBar, Rect(0.0f, 0.0f, shape.size.x, mEditorTop));
        }
        const float editorY = mEditorTop;
        const float editorH = shape.size.y - editorY < 1.0f ? 1.0f : shape.size.y - editorY;
        if (mView != nullptr)
        {
            if (mHeaderBar != nullptr)
            {
                setBox(*mView, Rect(0.0f, editorY, shape.size.x, editorH));
            }
            else
            {
                setBox(*mView, Rect(0.0f, 0.0f, shape.size.x, shape.size.y));
            }
            mView->layout();
        }
        if (mStructEditor != nullptr && mStructEditor->parent() == this)
        {
            setBox(*mStructEditor, Rect(0.0f, editorY, shape.size.x, editorH));
            mStructEditor->layout();
        }
        if (mListGridEditor != nullptr && mListGridEditor->parent() == this)
        {
            setBox(*mListGridEditor, Rect(0.0f, editorY, shape.size.x, editorH));
            mListGridEditor->layout();
        }
        else if (mListGridEditor != nullptr && mListGridEditor->parent() != nullptr)
        {
            mListGridEditor->layout();
        }
        this->placeOverlayEditors();
        UIWidget::layout();
    }

    UIWidget* RoseInstanceDesigner::focusWidget() const
    {
        if (mStructEditor != nullptr && mStructEditor->parent() != nullptr)
        {
            if (UIWidget* focus = mStructEditor->focusWidget())
            {
                return focus;
            }
        }
        if (mListGridEditor != nullptr && mListGridEditor->parent() != nullptr)
        {
            if (UIWidget* focus = mListGridEditor->focusWidget())
            {
                return focus;
            }
        }
        return mFocus;
    }

    void RoseInstanceDesigner::placeOverlayEditors()
    {
        if (mListGridEditor != nullptr && mListGridEditor->parent() != nullptr)
        {
            mListGridEditor->placeOverlayEditors();
            return;
        }
        if (mStructEditor != nullptr && mStructEditor->parent() != nullptr)
        {
            mStructEditor->placeOverlayEditors();
            return;
        }
        if (mOverlay == nullptr)
        {
            return;
        }
        if (mPickMenu != nullptr && mPickAnchor != nullptr)
        {
            placePopup(*mOverlay, *mPickMenu, mPickAnchor->screenBounds());
        }
        if (mDatePanel != nullptr && mDateAnchor != nullptr)
        {
            placePopup(*mOverlay, *mDatePanel, mDateAnchor->screenBounds());
        }
        if (mColorPanel != nullptr && mColorAnchor != nullptr)
        {
            placePopup(*mOverlay, *mColorPanel, mColorAnchor->screenBounds());
        }
    }

    bool RoseInstanceDesigner::handleEscape()
    {
        if (mListGridEditor != nullptr && mListGridEditor->parent() != nullptr && mListGridEditor->handleEscape())
        {
            return true;
        }
        if (mStructEditor != nullptr && mStructEditor->parent() != nullptr && mStructEditor->handleEscape())
        {
            return true;
        }
        if (mColorPicker != nullptr && mColorPicker->eyeDropping())
        {
            mColorPicker->cancelEyeDropper();
            return true;
        }
        if (mEdit.kind != ValueEditState::Kind::None)
        {
            this->closeValueEdit();
            return true;
        }
        return false;
    }

    bool RoseInstanceDesigner::colorDropping() const
    {
        if (mListGridEditor != nullptr && mListGridEditor->parent() != nullptr && mListGridEditor->colorDropping())
        {
            return true;
        }
        if (mStructEditor != nullptr && mStructEditor->parent() != nullptr && mStructEditor->colorDropping())
        {
            return true;
        }
        return mColorPicker != nullptr && mColorPicker->eyeDropping();
    }

    void RoseInstanceDesigner::sampleColorDropper(float x, float y, bool commit)
    {
        if (mListGridEditor != nullptr && mListGridEditor->parent() != nullptr && mListGridEditor->colorDropping())
        {
            mListGridEditor->sampleColorDropper(x, y, commit);
            return;
        }
        if (mStructEditor != nullptr && mStructEditor->parent() != nullptr && mStructEditor->colorDropping())
        {
            mStructEditor->sampleColorDropper(x, y, commit);
            return;
        }
        if (mColorPicker != nullptr)
        {
            mColorPicker->sampleEyeDropper(x, y, commit);
        }
    }

    void RoseInstanceDesigner::cancelColorDropper()
    {
        if (mListGridEditor != nullptr && mListGridEditor->parent() != nullptr)
        {
            mListGridEditor->cancelColorDropper();
        }
        if (mStructEditor != nullptr && mStructEditor->parent() != nullptr)
        {
            mStructEditor->cancelColorDropper();
        }
        if (mColorPicker != nullptr)
        {
            mColorPicker->cancelEyeDropper();
        }
    }

    void RoseInstanceDesigner::beginValueEdit(ValueEditState::Kind kind, const NestedField& info, const String& shown)
    {
        mEdit.kind = kind;
        mEdit.fieldKind = info.kind;
        mEdit.options = info.options;
        if (kind == ValueEditState::Kind::Date)
        {
            initTemporalParts(info.kind, shown, mEdit.date);
        }
        this->notifyRebuild();
    }

    void RoseInstanceDesigner::closeValueEdit()
    {
        mEdit = {};
        this->notifyRebuild();
    }

    bool RoseInstanceDesigner::commitScalarText(const String& text)
    {
        if (mDoc == nullptr)
        {
            return false;
        }
        if (mDoc->instanceText() == text)
        {
            this->closeValueEdit();
            return true;
        }
        if (!mDoc->setInstanceText(text))
        {
            return false;
        }
        this->closeValueEdit();
        return true;
    }

    void RoseInstanceDesigner::commitScalarBool(bool value)
    {
        if (mDoc != nullptr)
        {
            mDoc->setInstanceText(value ? "true" : "false");
            this->notifyRebuild();
        }
    }

    void RoseInstanceDesigner::commitScalarLabels(const std::vector<String>& labels)
    {
        if (mDoc != nullptr)
        {
            mDoc->setInstanceLabels(labels);
            this->notifyRebuild();
        }
    }

    void RoseInstanceDesigner::buildScalarBody(UIWidget& body, float panelW, float bodyH)
    {
        const float s = this->scale();
        const float fontPx = bodyPx(s);
        const float smallPx = bodyPx(s);
        const RoseTheme& t = theme();
        const Color ink = t.ink;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const String& fontPath = this->font();
        NestedField info = mDoc->instanceRootField();
        String shown = mDoc->instanceText();
        const float valueH = 36.0f * s;

        auto viewer = std::make_shared<UIWidget>();
        viewer->shape.pivot = Vector2(0.0f, 0.0f);
        viewer->color = t.header;
        setBox(viewer, Rect(0.0f, 0.0f, panelW, valueH));
        Vector2 hitSize(viewer->shape.size.x, valueH);

        if (optionKind(info.kind))
        {
            std::vector<String> labels = mDoc->instanceLabels();
            auto openOption = [this, info, shown]() { this->beginValueEdit(ValueEditState::Kind::Option, info, shown); };
            attachOptionTags(*viewer, hitSize, s, info.kind, info.options, labels, fontPath, smallPx,
                [this, info](const String& label)
                {
                    if (info.kind == FieldKind::SingleOption)
                    {
                        this->commitScalarText(String::empty);
                        return;
                    }
                    std::vector<String> current = mDoc->instanceLabels();
                    this->commitScalarLabels(removeLabel(current, label));
                },
                openOption);
            viewer->onClick = openOption;
            viewer->onDoubleClick = openOption;
            if (mEdit.kind == ValueEditState::Kind::Option)
            {
                mPickAnchor = viewer.get();
            }
        }
        else if (info.kind == FieldKind::Color)
        {
            attachColorValue(*viewer, hitSize, s, shown, fontPath, fontPx);
            auto openColor = [this, info, shown]() { this->beginValueEdit(ValueEditState::Kind::Color, info, shown); };
            viewer->onClick = openColor;
            viewer->onDoubleClick = openColor;
            if (mEdit.kind == ValueEditState::Kind::Color)
            {
                mColorAnchor = viewer.get();
            }
        }
        else if (temporalKind(info.kind))
        {
            attachValueText(*viewer, hitSize, s, shown, fontPath, fontPx, ink);
            auto openDate = [this, info, shown]() { this->beginValueEdit(ValueEditState::Kind::Date, info, shown); };
            viewer->onClick = openDate;
            viewer->onDoubleClick = openDate;
            if (mEdit.kind == ValueEditState::Kind::Date)
            {
                mDateAnchor = viewer.get();
            }
        }
        else if (info.kind == FieldKind::Boolean)
        {
            auto toggle = makeBoolToggle(s, hitSize, boolFromText(shown));
            toggle->onValueChanged = [this](bool value) { this->commitScalarBool(value); };
            viewer->attachChild(toggle);
        }
        else if (pathKind(info.kind))
        {
            attachValueText(*viewer, hitSize, s, shown, fontPath, fontPx, ink);
            auto pickPath = [this, info, shown]()
            {
                String picked = String::empty;
                if (info.kind == FieldKind::FilePath && mHost.pickFilePath)
                {
                    picked = mHost.pickFilePath(shown);
                }
                else if (info.kind == FieldKind::FolderPath && mHost.pickFolderPath)
                {
                    picked = mHost.pickFolderPath(shown);
                }
                if (!picked.isEmpty())
                {
                    this->commitScalarText(picked);
                }
            };
            viewer->onClick = pickPath;
            viewer->onDoubleClick = pickPath;
            body.attachChild(viewer);
            (void)bodyH;
            return;
        }
        else
        {
            auto input = std::make_shared<EscapeInput>();
            input->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(input, Rect(0.0f, 0.0f, panelW, valueH));
            styleCellInput(*input, t.field, s, fontPath, fontPx, mHost.writeClipboard, mHost.readClipboard);
            if (info.kind == FieldKind::Url && !shown.isEmpty())
            {
                if (UIText* label = input->label())
                {
                    styleText(*label, fontPath, fontPx, t.url);
                }
            }
            input->setText(shown);
            UIInput* raw = input.get();
            input->onSubmit = [this](const String& text) { this->commitScalarText(text); };
            bindCommitOnFocusLoss(*input, mHost.isRebuildingUi, [this, raw]()
            {
                if (raw != nullptr)
                {
                    this->commitScalarText(raw->text);
                }
            });
            input->onEscape = [this]() { this->handleEscape(); };
            body.attachChild(input);
            mFocus = raw;
            (void)bodyH;
            return;
        }
        body.attachChild(viewer);
        (void)bodyH;
    }

    void RoseInstanceDesigner::rebuild()
    {
        this->clearOverlayEditors();
        this->detachChildren();
        mView = nullptr;
        mFocus = nullptr;
        mNameInput = nullptr;
        mHeaderBar = nullptr;
        mEditorTop = 0.0f;
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveInstance())
        {
            return;
        }

        const float s = this->scale();
        const float titlePx = titleMdPx(s);
        const float fontPx = bodyPx(s);
        const float smallPx = bodyPx(s);
        const float inset = kRoseContentInset * s;
        float nameRowH = 36.0f * s;
        if (nameRowH < titlePx + 12.0f * s)
        {
            nameRowH = titlePx + 12.0f * s;
        }
        const RoseTheme& t = theme();
        const Color paper = t.paper;
        const Color ink = t.ink;
        const Color accent = t.accent;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const Color field = t.field;
        const String& fontPath = this->font();
        const float width = shape.size.x > 1.0f ? shape.size.x : 400.0f * s;
        const float height = shape.size.y > 1.0f ? shape.size.y : 400.0f * s;
        fillSurface(*this, paper);
        this->shape.pivot = Vector2(0.0f, 0.0f);

        float cardW = width;
        if (cardW < 1.0f)
        {
            cardW = 1.0f;
        }

        auto makeCard = [&]()
        {
            auto card = std::make_shared<PaintPanel>();
            card->pickable = false;
            card->fill(t.raised);
            card->hair = t.hair;
            card->framed = true;
            return card;
        };

        auto styleField = [&](UIInput& input, float size)
        {
            input.paddingX = 8.0f * s;
            input.paddingY = 4.0f * s;
            input.background = field;
            input.hoverFill = hoverOf(field);
            input.border.color = t.ring;
            input.border.thickness = 1.0f;
            input.caret.color = ink;
            input.selection = t.selection;
            if (UIText* label = input.label())
            {
                styleText(*label, fontPath, size, ink);
            }
            input.writeClipboard = mHost.writeClipboard;
            input.readClipboard = mHost.readClipboard;
        };

        rose::Value* value = mDoc->activeInstanceValue();
        RoseInstanceKind kind = classifyInstanceKind(value != nullptr ? value->schema : nullptr);
        rose::SchemaType schemaType = rose::SchemaType::String;
        if (value != nullptr && value->schema != nullptr)
        {
            schemaType = value->schema->type();
        }
        u32_t activeIndex = mDoc->activeInstanceIndex();
        String schemaCaption = schemaLabel(mDoc->instanceSchemaName(activeIndex));
        if (schemaCaption.isEmpty())
        {
            schemaCaption = String(schemaTypeLabel(schemaType));
        }
        if (schemaCaption.isEmpty())
        {
            schemaCaption = schemaLabel(String("String"));
        }

        auto titleCard = makeCard();
        auto titleChip = makeChip(schemaCaption, fontPath, titlePx, withAlpha(accent, t.washAlpha), accent);
        titleChip->pickable = false;
        float chipW = titleChip->shape.size.x;
        float chipH = titleChip->shape.size.y;
        float titleBarH = inset * 2.0f + (nameRowH > chipH ? nameRowH : chipH);
        mEditorTop = titleBarH;
        setBox(titleCard, Rect(0.0f, 0.0f, width, titleBarH));
        setBox(titleChip, Rect(inset, (titleBarH - chipH) * 0.5f, chipW, chipH));
        titleCard->attachChild(titleChip);
        const float nameX = inset + chipW + 12.0f * s;
        float nameW = width - nameX - inset;
        if (nameW < 80.0f * s)
        {
            nameW = 80.0f * s;
        }
        auto nameInput = std::make_shared<EscapeInput>();
        nameInput->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(nameInput, Rect(nameX, (titleBarH - nameRowH) * 0.5f, nameW, nameRowH));
        styleField(*nameInput, titlePx);
        nameInput->setText(mDoc->instanceName(activeIndex));
        UIInput* nameRaw = nameInput.get();
        nameInput->onSubmit = [this, activeIndex](const String& text)
        {
            if (mDoc != nullptr)
            {
                mDoc->renameInstance(activeIndex, text);
                this->notifyRebuild();
            }
        };
        bindCommitOnFocusLoss(*nameInput, mHost.isRebuildingUi, [this, activeIndex, nameRaw]()
        {
            if (mDoc != nullptr && nameRaw != nullptr)
            {
                mDoc->renameInstance(activeIndex, nameRaw->text);
            }
        });
        titleCard->attachChild(nameInput);
        mNameInput = nameInput.get();
        this->attachChild(titleCard);
        mHeaderBar = titleCard.get();

        float editorY = titleBarH;
        float editorH = height - titleBarH;
        if (editorH < 1.0f)
        {
            editorH = 1.0f;
        }

        if (kind == RoseInstanceKind::Struct)
        {
            if (!mStructEditor)
            {
                mStructEditor = std::make_shared<RoseInstanceEditor_Struct>();
            }
            RoseInstanceEditor_Struct::Host sh;
            sh.requestRebuild = mHost.requestRebuild;
            sh.isRebuildingUi = mHost.isRebuildingUi;
            sh.writeClipboard = mHost.writeClipboard;
            sh.readClipboard = mHost.readClipboard;
            sh.openExternal = mHost.openExternal;
            sh.pickFilePath = mHost.pickFilePath;
            sh.pickFolderPath = mHost.pickFolderPath;
            sh.ctrlDown = mHost.ctrlDown;
            sh.overlay = mOverlay;
            sh.font = mHost.font;
            sh.scale = mHost.scale;
            mStructEditor->shape.pivot = Vector2(0.0f, 0.0f);
            mStructEditor->bind(*mDoc, sh);
            mStructEditor->setOverlay(mOverlay);
            mStructEditor->setScroll(mKeepY);
            setBox(mStructEditor, Rect(0.0f, editorY, width, editorH));
            mStructEditor->rebuild();
            this->attachChild(mStructEditor);
        }
        else if (kind == RoseInstanceKind::ListElement || kind == RoseInstanceKind::ListOptions)
        {
            if (!mListGridEditor)
            {
                mListGridEditor = std::make_shared<RoseInstanceEditor_ListGrid>();
            }
            RoseInstanceEditor_ListGrid::Host lh;
            lh.requestRebuild = mHost.requestRebuild;
            lh.isRebuildingUi = mHost.isRebuildingUi;
            lh.writeClipboard = mHost.writeClipboard;
            lh.readClipboard = mHost.readClipboard;
            lh.openExternal = mHost.openExternal;
            lh.pickFilePath = mHost.pickFilePath;
            lh.pickFolderPath = mHost.pickFolderPath;
            lh.ctrlDown = mHost.ctrlDown;
            lh.overlay = mOverlay;
            lh.font = mHost.font;
            lh.scale = mHost.scale;
            mListGridEditor->bind(*mDoc, lh);
            mListGridEditor->shape.pivot = Vector2(0.0f, 0.0f);
            mListGridEditor->setOverlay(mOverlay);
            mListGridEditor->setScroll(0.0f, mKeepY);
            setBox(mListGridEditor, Rect(0.0f, editorY, width, editorH));
            mListGridEditor->rebuild();
            this->attachChild(mListGridEditor);
        }
        else
        {
            auto view = std::make_shared<UIView>();
            view->shape.pivot = Vector2(0.0f, 0.0f);
            fillSurface(*view, paper);
            setBox(view, Rect(0.0f, 0.0f, width, editorH));

            auto stack = std::make_shared<UIList>();
            stack->direction = UIDirection::Vertical;
            stack->padding = 0.0f;
            stack->spacing = 0.0f;
            stack->color = clear;
            stack->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(stack, Rect(0.0f, 0.0f, width, 0.0f));

            const float valueH = 36.0f * s;
            const float contentH = valueH;
            const float innerW = cardW - inset * 2.0f;
            auto bodyCard = makeCard();
            auto body = std::make_shared<UIWidget>();
            body->shape.pivot = Vector2(0.0f, 0.0f);
            body->color = clear;
            body->pickable = false;
            setBox(body, Rect(inset, inset, innerW, contentH));
            this->buildScalarBody(*body, innerW, contentH);
            body->layout();
            setBox(bodyCard, Rect(0.0f, 0.0f, cardW, contentH + inset * 2.0f));
            bodyCard->attachChild(body);
            stack->attachChild(bodyCard);

            stack->layout();
            view->addChild(stack);
            view->setScroll(0.0f, mKeepY);
            this->attachChild(view);
            mView = view.get();
        }

        if (mView != nullptr && mOverlay != nullptr && mEdit.kind == ValueEditState::Kind::Option && mPickAnchor != nullptr)
        {
            NestedField info = mDoc->instanceRootField();
            FieldKind pickedFieldKind = mEdit.fieldKind;
            auto menu = makeOptionMenu(
                mEdit.options,
                mDoc->instanceLabels(),
                s,
                fontPath,
                fontPx,
                mPickAnchor->shape.size.x,
                mPickAnchor,
                [this, pickedFieldKind](const String& label)
                {
                    if (pickedFieldKind == FieldKind::MultiOption)
                    {
                        std::vector<String> current = mDoc->instanceLabels();
                        if (selectedContains(current, label))
                        {
                            this->commitScalarLabels(removeLabel(current, label));
                        }
                        else
                        {
                            current.push_back(label);
                            this->commitScalarLabels(current);
                        }
                    }
                    else
                    {
                        mEdit = {};
                        this->commitScalarText(label);
                    }
                },
                [this]() { this->closeValueEdit(); });
            mOverlay->attachChild(menu);
            mPickMenu = menu.get();
            mFocus = menu.get();
        }

        if (mView != nullptr && mOverlay != nullptr && mEdit.kind == ValueEditState::Kind::Date && mDateAnchor != nullptr)
        {
            FieldKind dateKind = mEdit.fieldKind;
            DatePanelWidgets widgets = makeDatePanel(
                dateKind,
                mEdit.date,
                s,
                fontPath,
                fontPx,
                smallPx,
                [this]() { this->closeValueEdit(); },
                [this]()
                {
                    mEdit = {};
                    this->commitScalarText(String::empty);
                },
                [this](const String& stored)
                {
                    mEdit = {};
                    this->commitScalarText(stored);
                },
                [this]() { this->handleEscape(); },
                [this](const TemporalParts& next) { mEdit.date = next; });
            mOverlay->attachChild(widgets.panel);
            mDatePanel = widgets.panel.get();
            mFocus = widgets.focus;
        }

        if (mView != nullptr && mOverlay != nullptr && mEdit.kind == ValueEditState::Kind::Color && mColorAnchor != nullptr)
        {
            Color current(1.0f, 1.0f, 1.0f, 1.0f);
            parseColor(mDoc->instanceText(), current);
            ColorPanelWidgets widgets = makeColorPanel(
                current,
                s,
                fontPath,
                fontPx,
                smallPx,
                mHost.writeClipboard,
                mHost.readClipboard,
                [this](const Color& value)
                {
                    mEdit = {};
                    this->commitScalarText(formatColor(value));
                },
                [this]()
                {
                    mEdit = {};
                    this->commitScalarText(String::empty);
                },
                [this]() { this->closeValueEdit(); },
                [this]() { this->handleEscape(); });
            mOverlay->attachChild(widgets.panel);
            mColorPanel = widgets.panel.get();
            mColorPicker = widgets.picker;
            mFocus = widgets.focus;
        }

        if (mStructEditor != nullptr && mStructEditor->parent() != nullptr)
        {
            if (UIWidget* focus = mStructEditor->focusWidget())
            {
                mFocus = focus;
            }
        }
        else if (mListGridEditor != nullptr && mListGridEditor->parent() != nullptr)
        {
            if (UIWidget* focus = mListGridEditor->focusWidget())
            {
                mFocus = focus;
            }
        }
        else if (mFocus == nullptr)
        {
            mFocus = mNameInput;
        }
    }

}