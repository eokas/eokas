#include "./InstanceEditor_Struct.h"
#include "./FieldEditors.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"

#include <cmath>
#include <functional>
#include <utility>
#include <vector>

namespace eokas::RoseAxion {

    namespace {

        String foldKey(const std::vector<NestedHop>& path)
        {
            String key;
            for (const NestedHop& hop : path)
            {
                if (hop.list)
                {
                    key += String::format("/[%u]", hop.index);
                }
                else
                {
                    key += "/";
                    key += hop.member;
                }
            }
            return key;
        }

        float atLeast(float value, float floor)
        {
            return value < floor ? floor : value;
        }

        bool samePath(const std::vector<NestedHop>& a, const std::vector<NestedHop>& b)
        {
            if (a.size() != b.size())
            {
                return false;
            }
            for (size_t i = 0; i < a.size(); ++i)
            {
                if (a[i].list != b[i].list)
                {
                    return false;
                }
                if (a[i].list)
                {
                    if (a[i].index != b[i].index)
                    {
                        return false;
                    }
                }
                else if (a[i].member != b[i].member)
                {
                    return false;
                }
            }
            return true;
        }

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

    void RoseInstanceEditor_Struct::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseInstanceEditor_Struct::setOverlay(UIWidget* overlay)
    {
        mOverlay = overlay;
        mHost.overlay = overlay;
    }

    void RoseInstanceEditor_Struct::reset()
    {
        mEdit = {};
        mKeepY = 0.0f;
        mView = nullptr;
        mFocus = nullptr;
        mHasFocusTarget = false;
        mFocusPath.clear();
        mFocusCol = 0;
        this->clearOverlayEditors();
        this->detachChildren();
    }

    const String& RoseInstanceEditor_Struct::font() const
    {
        return mHost.font;
    }

    float RoseInstanceEditor_Struct::scale() const
    {
        return hostScale(mHost.scale);
    }

    void RoseInstanceEditor_Struct::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    void RoseInstanceEditor_Struct::refreshUi()
    {
        this->notifyRebuild();
    }

    float RoseInstanceEditor_Struct::scrollY() const
    {
        return mView != nullptr ? mView->scrollY() : mKeepY;
    }

    void RoseInstanceEditor_Struct::setScroll(float y)
    {
        mKeepY = y;
        if (mView != nullptr)
        {
            mView->setScroll(0.0f, y);
        }
    }

    void RoseInstanceEditor_Struct::layout()
    {
        if (!visible)
        {
            return;
        }
        if (mView != nullptr)
        {
            setBox(*mView, Rect(0.0f, 0.0f, shape.size.x, shape.size.y));
            mView->layout();
        }
        this->placeOverlayEditors();
        UIWidget::layout();
    }

    void RoseInstanceEditor_Struct::clearOverlayEditors()
    {
        UIWidget* overlay = mOverlay != nullptr ? mOverlay : mHost.overlay;
        if (overlay != nullptr)
        {
            if (mPickMenu != nullptr)
            {
                overlay->detachChild(mPickMenu);
            }
            if (mDatePanel != nullptr)
            {
                overlay->detachChild(mDatePanel);
            }
            if (mColorPanel != nullptr)
            {
                overlay->detachChild(mColorPanel);
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

    void RoseInstanceEditor_Struct::placeOverlayEditors()
    {
        UIWidget* overlay = mOverlay != nullptr ? mOverlay : mHost.overlay;
        if (overlay == nullptr)
        {
            return;
        }
        if (mPickMenu != nullptr && mPickAnchor != nullptr)
        {
            placePopup(*overlay, *mPickMenu, mPickAnchor->screenBounds());
        }
        if (mDatePanel != nullptr && mDateAnchor != nullptr)
        {
            placePopup(*overlay, *mDatePanel, mDateAnchor->screenBounds());
        }
        if (mColorPanel != nullptr && mColorAnchor != nullptr)
        {
            placePopup(*overlay, *mColorPanel, mColorAnchor->screenBounds());
        }
    }

    bool RoseInstanceEditor_Struct::handleEscape()
    {
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

    bool RoseInstanceEditor_Struct::colorDropping() const
    {
        return mColorPicker != nullptr && mColorPicker->eyeDropping();
    }

    void RoseInstanceEditor_Struct::sampleColorDropper(float x, float y, bool commit)
    {
        if (mColorPicker != nullptr)
        {
            mColorPicker->sampleEyeDropper(x, y, commit);
        }
    }

    void RoseInstanceEditor_Struct::cancelColorDropper()
    {
        if (mColorPicker != nullptr)
        {
            mColorPicker->cancelEyeDropper();
        }
    }

    void RoseInstanceEditor_Struct::closeValueEdit()
    {
        mEdit = {};
        this->refreshUi();
    }

    void RoseInstanceEditor_Struct::beginValueEdit(ValueEditState::Kind kind, const std::vector<NestedHop>& path, const NestedField& info, const String& shown)
    {
        mEdit.kind = kind;
        mEdit.path = path;
        mEdit.fieldKind = info.kind;
        mEdit.options = info.options;
        if (kind == ValueEditState::Kind::Date)
        {
            initTemporalParts(info.kind, shown, mEdit.date);
        }
        this->refreshUi();
    }

    bool RoseInstanceEditor_Struct::editingField(ValueEditState::Kind kind, const std::vector<NestedHop>& path) const
    {
        return mEdit.kind == kind && samePath(mEdit.path, path);
    }

    void RoseInstanceEditor_Struct::commitText(const std::vector<NestedHop>& path, const String& text)
    {
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveInstance())
        {
            return;
        }
        if (mDoc->instanceStructText(path) == text)
        {
            mEdit = {};
            this->refreshUi();
            return;
        }
        mDoc->setInstanceStructText(path, text);
        this->closeValueEdit();
    }

    void RoseInstanceEditor_Struct::commitBool(const std::vector<NestedHop>& path, bool value)
    {
        this->commitText(path, value ? "true" : "false");
    }

    void RoseInstanceEditor_Struct::commitLabels(const std::vector<NestedHop>& path, const std::vector<String>& labels)
    {
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveInstance())
        {
            return;
        }
        mDoc->setInstanceStructLabels(path, labels);
        this->closeValueEdit();
    }

    void RoseInstanceEditor_Struct::rebuild()
    {
        this->clearOverlayEditors();
        this->detachChildren();
        mView = nullptr;
        mFocus = nullptr;
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveInstance())
        {
            return;
        }
        if (!mDoc->hasActiveInstance())
        {
            return;
        }

        const float s = this->scale();
        const float fontPx = bodyPx(s);
        const float smallPx = bodyPx(s);
        const RoseTheme& t = theme();
        const Color ink = t.ink;
        const Color mute = t.mute;
        const Color quiet = t.quiet;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const Color grid = t.gridLine.a > 0.0f ? t.gridLine : t.line;
        const Color groupFill = t.header;
        const Color leafFill = t.raised;
        const String& fontPath = this->font();
        const float panelW = shape.size.x > 1.0f ? shape.size.x : 420.0f * s;
        const float headPad = 12.0f * s;
        const float headH = 44.0f * s;
        const float closeW = 64.0f * s;
        const float treeRowH = 32.0f * s;
        const float treeIndent = 18.0f * s;
        const float delW = 36.0f * s;
        this->shape.pivot = Vector2(0.0f, 0.0f);
        this->color = t.paper;
        this->pickable = true;

        auto fitRowButton = [&](const std::shared_ptr<UIButton>& button, float height)
        {
            if (!button)
            {
                return;
            }
            button->paddingY = 2.0f * s;
            float width = button->shape.size.x;
            if (width < 1.0f)
            {
                width = 1.0f;
            }
            setBox(button, Rect(0.0f, 0.0f, width, height));
        };

        const float panelH = shape.size.y > 1.0f ? shape.size.y : 400.0f * s;

        auto view = std::make_shared<UIView>();
        view->shape.pivot = Vector2(0.0f, 0.0f);
        view->color = t.paper;
        setBox(view, Rect(0.0f, 0.0f, panelW, panelH));

        auto body = std::make_shared<UIList>();
        body->direction = UIDirection::Vertical;
        body->padding = 0.0f;
        body->spacing = 0.0f;
        body->color = clear;
        body->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(body, Rect(0.0f, 0.0f, panelW, 0.0f));

        const u32_t cols = mDoc->instanceStructMemberCount();
        if (cols == 0)
        {
            auto empty = makeText("No fields", fontPath, fontPx, mute, panelW - headPad * 2.0f, 22.0f * s);
            empty->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(empty, Rect(headPad, 8.0f * s, panelW - headPad * 2.0f, 22.0f * s));
            body->attachChild(empty);
            body->layout();
            view->addChild(body);
            this->attachChild(view);
            mView = view.get();
            view->setScroll(0.0f, mKeepY);
            view->layout();
            return;
        }

        float nameW = panelW * 0.42f;
        nameW = atLeast(nameW, 120.0f * s);
        float minValue = 80.0f * s;
        if (nameW > panelW - minValue)
        {
            nameW = atLeast(panelW - minValue, 80.0f * s);
        }
        float valueW = atLeast(panelW - nameW, minValue);
        float nameInnerH = atLeast(treeRowH - 1.0f, 1.0f);
        float valueInnerW = atLeast(valueW - 2.0f, 1.0f);
        float valueInnerH = atLeast(treeRowH - 1.0f, 1.0f);

        auto tree = std::make_shared<UITable>();
        tree->shape.pivot = Vector2(0.0f, 0.0f);
        tree->color = t.paper;
        tree->cellSpacing = 0.0f;
        tree->cellPadding = 0.0f;
        tree->border = UITableBorder(1.0f, 1.0f, 0.0f, 0.0f, grid);
        tree->disclosure = mute;
        tree->addColumn(nameW);
        tree->addColumn(valueW);
        tree->setColumnIndent(0, treeIndent);

        auto paintLine = [&](UITableRow* line, bool group)
        {
            Color fill = group ? groupFill : leafFill;
            for (int column = 0; column < 2; ++column)
            {
                UITableCell* item = line->cell(column);
                if (item == nullptr)
                {
                    continue;
                }
                item->color = fill;
                float right = column == 1 ? 1.0f : 0.0f;
                item->setBorder(UITableBorder(1.0f, 1.0f, right, 0.0f, grid));
            }
        };
        auto bindName = [&](UITableRow* line, const String& name, bool group)
        {
            float shift = static_cast<float>(line->depth() + 1) * treeIndent;
            float innerW = atLeast(nameW - 1.0f - shift, 1.0f);
            auto cell = std::make_shared<UIWidget>();
            cell->pickable = false;
            cell->shape.pivot = Vector2(0.0f, 0.0f);
            cell->color = clear;
            setBox(cell, Rect(0.0f, 0.0f, innerW, nameInnerH));
            float textPad = 6.0f * s;
            float textW = atLeast(innerW - textPad, 1.0f);
            auto label = makeText(name, fontPath, smallPx, group ? mute : ink, textW, smallPx);
            float labelH = label->shape.size.y;
            if (labelH < 1.0f)
            {
                labelH = smallPx;
            }
            float y = (nameInnerH - labelH) * 0.5f;
            if (y < 0.0f)
            {
                y = 0.0f;
            }
            setBox(label, Rect(textPad, y, textW, labelH));
            cell->attachChild(label);
            line->setContent(0, cell);
        };
        auto bindFold = [this](UITableRow* line, u32_t col, const std::vector<NestedHop>& path)
        {
            String key = foldKey(path);
            line->onExpandedChanged = [this, key](bool expanded)
            {
                if (expanded)
                {
                    mCollapsed.erase(key);
                }
                else
                {
                    mCollapsed.insert(key);
                }
            };
            if (mCollapsed.find(key) != mCollapsed.end())
            {
                line->setExpanded(false);
            }
        };

        auto matchesFocus = [this](u32_t col, const std::vector<NestedHop>& path)
        {
            return mHasFocusTarget && samePath(path, mFocusPath) && col == mFocusCol;
        };
        auto trackFocus = [this](UIWidget& widget, u32_t col, const std::vector<NestedHop>& path)
        {
            widget.onFocusGain = [this, path, col]()
            {
                mFocusPath = path;
                mFocusCol = col;
                mHasFocusTarget = true;
            };
        };

        std::function<UIWidget::Ref(u32_t, const std::vector<NestedHop>&, const NestedField&, const String&, float)> makeEditor;
        makeEditor = [&](u32_t col, const std::vector<NestedHop>& path, const NestedField& info, const String& shown, float editorW) -> UIWidget::Ref
        {
            Vector2 hitSize(editorW, valueInnerH);
            auto hit = std::make_shared<UIWidget>();
            hit->shape.pivot = Vector2(0.0f, 0.0f);
            hit->color = clear;
            hit->focusable = true;
            setBox(hit, Rect(0.0f, 0.0f, editorW, valueInnerH));
            FieldKind kind = info.kind;

            if (kind == FieldKind::Boolean)
            {
                auto toggle = makeBoolToggle(s, hitSize, boolFromText(shown));
                toggle->onValueChanged = [this, col, path](bool value) { this->commitBool(path, value); };
                hit->attachChild(toggle);
                trackFocus(*toggle, col, path);
                if (matchesFocus(col, path))
                {
                    mFocus = toggle.get();
                }
                return hit;
            }

            if (optionKind(kind))
            {
                std::vector<String> labels;
                if (kind == FieldKind::MultiOption)
                {
                    labels = path.empty()
                        ? mDoc->instanceStructLabels(path)
                        : mDoc->instanceStructLabels(path);
                }
                else if (!shown.isEmpty())
                {
                    labels.push_back(shown);
                }
                attachOptionTags(*hit, hitSize, s, kind, info.options, labels, fontPath, smallPx,
                    [this, col, path, kind](const String& label)
                    {
                        if (kind == FieldKind::SingleOption)
                        {
                            this->commitText(path, String::empty);
                            return;
                        }
                        std::vector<String> current = path.empty()
                            ? mDoc->instanceStructLabels(path)
                            : mDoc->instanceStructLabels(path);
                        this->commitLabels(path, removeLabel(current, label));
                    },
                    [this, col, path, info, shown]()
                    {
                        this->beginValueEdit(ValueEditState::Kind::Option, path, info, shown);
                    });
                auto openOption = [this, path, info, shown]()
                {
                    this->beginValueEdit(ValueEditState::Kind::Option, path, info, shown);
                };
                hit->onClick = openOption;
                hit->onDoubleClick = openOption;
                if (this->editingField(ValueEditState::Kind::Option, path))
                {
                    mPickAnchor = hit.get();
                }
                trackFocus(*hit, col, path);
                if (matchesFocus(col, path))
                {
                    mFocus = hit.get();
                }
                return hit;
            }

            if (temporalKind(kind))
            {
                attachValueText(*hit, hitSize, s, shown, fontPath, fontPx, ink);
                hit->onClick = [this, col, path, info, shown]()
                {
                    this->beginValueEdit(ValueEditState::Kind::Date, path, info, shown);
                };
                if (this->editingField(ValueEditState::Kind::Date, path))
                {
                    mDateAnchor = hit.get();
                }
                trackFocus(*hit, col, path);
                if (matchesFocus(col, path))
                {
                    mFocus = hit.get();
                }
                return hit;
            }

            if (kind == FieldKind::Color)
            {
                attachColorValue(*hit, hitSize, s, shown, fontPath, fontPx);
                hit->onClick = [this, col, path, info, shown]()
                {
                    this->beginValueEdit(ValueEditState::Kind::Color, path, info, shown);
                };
                if (this->editingField(ValueEditState::Kind::Color, path))
                {
                    mColorAnchor = hit.get();
                }
                trackFocus(*hit, col, path);
                if (matchesFocus(col, path))
                {
                    mFocus = hit.get();
                }
                return hit;
            }

            if (pathKind(kind))
            {
                attachValueText(*hit, hitSize, s, shown, fontPath, fontPx, ink);
                hit->onClick = [this, col, path, kind, shown]()
                {
                    String picked = String::empty;
                    if (kind == FieldKind::FilePath && mHost.pickFilePath)
                    {
                        picked = mHost.pickFilePath(shown);
                    }
                    else if (kind == FieldKind::FolderPath && mHost.pickFolderPath)
                    {
                        picked = mHost.pickFolderPath(shown);
                    }
                    if (!picked.isEmpty())
                    {
                        this->commitText(path, picked);
                    }
                };
                trackFocus(*hit, col, path);
                if (matchesFocus(col, path))
                {
                    mFocus = hit.get();
                }
                return hit;
            }

            auto input = std::make_shared<EscapeInput>();
            input->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(input, Rect(0.0f, 0.0f, editorW, valueInnerH));
            styleCellInput(*input, leafFill, s, fontPath, fontPx, mHost.writeClipboard, mHost.readClipboard);
            if (kind == FieldKind::Url && !shown.isEmpty())
            {
                if (UIText* label = input->label())
                {
                    styleText(*label, fontPath, fontPx, t.url);
                }
            }
            input->setText(shown);
            UIInput* raw = input.get();
            bindCommitOnFocusLoss(*input, mHost.isRebuildingUi, [this, raw, path]()
            {
                if (raw != nullptr)
                {
                    this->commitText(path, raw->text);
                }
            });
            trackFocus(*input, col, path);
            input->onSubmit = [this, col, path](const String& text) { this->commitText(path, text); };
            input->onEscape = [this]() { this->handleEscape(); };
            if (kind == FieldKind::Url)
            {
                input->onPointerPress = [this, shown](const UIPointerEvent&)
                {
                    if (mHost.ctrlDown && mHost.ctrlDown() && mHost.openExternal && !shown.isEmpty())
                    {
                        mHost.openExternal(shown);
                    }
                };
            }
            hit->attachChild(input);
            if (matchesFocus(col, path))
            {
                mFocus = raw;
            }
            return hit;
        };

        auto wrapDelete = [&](UIWidget::Ref editor, u32_t col, const std::vector<NestedHop>& path, float editorW) -> UIWidget::Ref
        {
            bool listItem = !path.empty() && path.back().list;
            if (!listItem)
            {
                return editor;
            }
            auto row = std::make_shared<UIList>();
            row->direction = UIDirection::Horizontal;
            row->padding = 0.0f;
            row->spacing = 6.0f * s;
            row->centerCross = true;
            row->color = clear;
            row->shape.pivot = Vector2(0.0f, 0.0f);
            float boxH = valueInnerH;
            if (editor && editor->shape.size.y <= 0.0f)
            {
                boxH = 0.0f;
            }
            else if (editor && editor->shape.size.y > boxH)
            {
                boxH = editor->shape.size.y;
            }
            setBox(row, Rect(0.0f, 0.0f, editorW, boxH));
            if (editor)
            {
                row->attachChild(editor);
            }
            auto del = makeDangerButton("Del", fontPath, smallPx, delW, valueInnerH);
            fitRowButton(del, valueInnerH);
            u32_t itemIndex = path.back().index;
            del->onClick = [this, col, path, itemIndex]()
            {
                std::vector<NestedHop> listPath(path.begin(), path.end() - 1);
                mDoc->instanceStructListRemove(listPath, itemIndex);
                this->refreshUi();
            };
            row->attachChild(del);
            return row;
        };

        std::function<void(UITableRow*, u32_t, const std::vector<NestedHop>&, const NestedField&)> attachField;
        attachField = [&](UITableRow* parent, u32_t col, const std::vector<NestedHop>& path, const NestedField& info)
        {
            String shown = path.empty()
                ? mDoc->instanceStructText(path)
                : mDoc->instanceStructText(path);
            bool nested = (info.nestedStruct || info.nestedList) && path.size() < 8;
            float rowH = treeRowH;
            UITableRow* line = parent != nullptr ? parent->addRow(rowH) : tree->addRow(rowH);
            paintLine(line, nested);
            bindName(line, info.name, nested);

            bool listItem = !path.empty() && path.back().list;
            float actionGap = listItem ? delW + 6.0f * s : 0.0f;
            float editorW = atLeast(valueInnerW - actionGap, 1.0f);

            if (info.nestedStruct && path.size() < 8)
            {
                bindFold(line, col, path);
                if (listItem)
                {
                    line->setContent(1, wrapDelete(nullptr, col, path, valueInnerW));
                }
                u32_t members = mDoc->instanceNestedFieldCount(path);
                if (members == 0)
                {
                    UITableRow* empty = line->addRow(treeRowH);
                    paintLine(empty, false);
                    bindName(empty, "No members", true);
                }
                for (u32_t index = 0; index < members; ++index)
                {
                    NestedField child = mDoc->instanceNestedField(path, index);
                    std::vector<NestedHop> childPath = path;
                    childPath.push_back(NestedHop::field(child.id));
                    attachField(line, col, childPath, child);
                }
                return;
            }
            if (info.nestedList && path.size() < 8)
            {
                bindFold(line, col, path);
                auto add = makeButton("Add Item", fontPath, smallPx, 96.0f * s, valueInnerH, quiet, ink);
                fitRowButton(add, valueInnerH);
                add->onClick = [this, col, path]()
                {
                    u32_t count = mDoc->instanceNestedListCount(path);
                    mDoc->instanceStructListInsert(path, count);
                    this->refreshUi();
                };
                UIWidget::Ref value = add;
                if (listItem)
                {
                    value = wrapDelete(add, col, path, valueInnerW);
                }
                line->setContent(1, value);
                u32_t count = mDoc->instanceNestedListCount(path);
                if (count == 0)
                {
                    UITableRow* empty = line->addRow(treeRowH);
                    paintLine(empty, false);
                    bindName(empty, "No items", true);
                }
                for (u32_t index = 0; index < count; ++index)
                {
                    NestedField item = mDoc->instanceNestedListElement(path);
                    item.name = String::format("Item %u", index + 1);
                    item.listItem = true;
                    std::vector<NestedHop> itemPath = path;
                    itemPath.push_back(NestedHop::at(index));
                    attachField(line, col, itemPath, item);
                }
                return;
            }

            UIWidget::Ref editor = makeEditor(col, path, info, shown, editorW);
            line->setContent(1, wrapDelete(editor, col, path, valueInnerW));
        };

        for (u32_t index = 0; index < cols; ++index)
        {
            NestedField child = mDoc->instanceStructMember(index);
            attachField(nullptr, index, { NestedHop::field(child.id) }, child);
        }

        setBox(tree, Rect(0.0f, 0.0f, panelW, 0.0f));
        tree->relayout();
        body->attachChild(tree);
        body->layout();
        view->addChild(body);
        this->attachChild(view);
        mView = view.get();
        view->setScroll(0.0f, mKeepY);
        view->layout();

        UIWidget* overlay = mOverlay != nullptr ? mOverlay : mHost.overlay;
        if (overlay != nullptr && mEdit.kind == ValueEditState::Kind::Option && mPickAnchor != nullptr)
        {
            std::vector<String> selected = mEdit.path.empty()
                ? mDoc->instanceStructLabels(mEdit.path)
                : mDoc->instanceStructLabels(mEdit.path);
            std::vector<NestedHop> path = mEdit.path;
            FieldKind kind = mEdit.fieldKind;
            auto menu = makeOptionMenu(
                mEdit.options,
                selected,
                s,
                fontPath,
                fontPx,
                mPickAnchor->shape.size.x,
                mPickAnchor,
                [this, path, kind](const String& label)
                {
                    if (kind == FieldKind::MultiOption)
                    {
                        std::vector<String> current = path.empty()
                            ? mDoc->instanceStructLabels(path)
                            : mDoc->instanceStructLabels(path);
                        if (selectedContains(current, label))
                        {
                            this->commitLabels(path, removeLabel(current, label));
                        }
                        else
                        {
                            current.push_back(label);
                            this->commitLabels(path, current);
                        }
                    }
                    else
                    {
                        mEdit = {};
                        this->commitText(path, label);
                    }
                },
                [this]() { this->closeValueEdit(); });
            overlay->attachChild(menu);
            mPickMenu = menu.get();
            mFocus = menu.get();
        }

        if (overlay != nullptr && mEdit.kind == ValueEditState::Kind::Date && mDateAnchor != nullptr)
        {
            FieldKind dateKind = mEdit.fieldKind;
            std::vector<NestedHop> path = mEdit.path;
            DatePanelWidgets widgets = makeDatePanel(
                dateKind,
                mEdit.date,
                s,
                fontPath,
                fontPx,
                smallPx,
                [this]() { this->closeValueEdit(); },
                [this, path]()
                {
                    mEdit = {};
                    this->commitText(path, String::empty);
                },
                [this, path](const String& stored)
                {
                    mEdit = {};
                    this->commitText(path, stored);
                },
                [this]() { this->handleEscape(); },
                [this](const TemporalParts& next) { mEdit.date = next; });
            overlay->attachChild(widgets.panel);
            mDatePanel = widgets.panel.get();
            mFocus = widgets.focus;
        }

        if (overlay != nullptr && mEdit.kind == ValueEditState::Kind::Color && mColorAnchor != nullptr)
        {
            Color current(1.0f, 1.0f, 1.0f, 1.0f);
            String shown = mEdit.path.empty()
                ? mDoc->instanceStructText(mEdit.path)
                : mDoc->instanceStructText(mEdit.path);
            parseColor(shown, current);
            std::vector<NestedHop> path = mEdit.path;
            ColorPanelWidgets widgets = makeColorPanel(
                current,
                s,
                fontPath,
                fontPx,
                smallPx,
                mHost.writeClipboard,
                mHost.readClipboard,
                [this, path](const Color& value)
                {
                    mEdit = {};
                    this->commitText(path, formatColor(value));
                },
                [this, path]()
                {
                    mEdit = {};
                    this->commitText(path, String::empty);
                },
                [this]() { this->closeValueEdit(); },
                [this]() { this->handleEscape(); });
            overlay->attachChild(widgets.panel);
            mColorPanel = widgets.panel.get();
            mColorPicker = widgets.picker;
            mFocus = widgets.focus;
        }
    }

}
