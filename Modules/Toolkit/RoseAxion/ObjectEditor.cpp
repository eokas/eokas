#include "./ObjectEditor.h"
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

        String foldKey(u32_t column, const std::vector<NestedHop>& path)
        {
            String key = String::format("%u", column);
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

    void RoseObjectEditor::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseObjectEditor::setRow(int row)
    {
        mRow = row;
    }

    void RoseObjectEditor::setColumn(int col)
    {
        mColumn = col;
    }

    const String& RoseObjectEditor::font() const
    {
        return mHost.font;
    }

    float RoseObjectEditor::scale() const
    {
        return hostScale(mHost.scale);
    }

    void RoseObjectEditor::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    float RoseObjectEditor::scrollY() const
    {
        return mView != nullptr ? mView->scrollY() : mKeepY;
    }

    void RoseObjectEditor::setScroll(float y)
    {
        mKeepY = y;
        if (mView != nullptr)
        {
            mView->setScroll(0.0f, y);
        }
    }

    void RoseObjectEditor::layout()
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

    void RoseObjectEditor::clearOverlayEditors()
    {
        if (mHost.overlay != nullptr)
        {
            if (mPickMenu != nullptr)
            {
                mHost.overlay->detachChild(mPickMenu);
            }
            if (mDatePanel != nullptr)
            {
                mHost.overlay->detachChild(mDatePanel);
            }
            if (mColorPanel != nullptr)
            {
                mHost.overlay->detachChild(mColorPanel);
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

    void RoseObjectEditor::placeOverlayEditors()
    {
        if (mHost.overlay == nullptr)
        {
            return;
        }
        if (mPickMenu != nullptr && mPickAnchor != nullptr)
        {
            placePopup(*mHost.overlay, *mPickMenu, mPickAnchor->screenBounds());
        }
        if (mDatePanel != nullptr && mDateAnchor != nullptr)
        {
            placePopup(*mHost.overlay, *mDatePanel, mDateAnchor->screenBounds());
        }
        if (mColorPanel != nullptr && mColorAnchor != nullptr)
        {
            placePopup(*mHost.overlay, *mColorPanel, mColorAnchor->screenBounds());
        }
    }

    bool RoseObjectEditor::handleEscape()
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

    bool RoseObjectEditor::colorDropping() const
    {
        return mColorPicker != nullptr && mColorPicker->eyeDropping();
    }

    void RoseObjectEditor::sampleColorDropper(float x, float y, bool commit)
    {
        if (mColorPicker != nullptr)
        {
            mColorPicker->sampleEyeDropper(x, y, commit);
        }
    }

    void RoseObjectEditor::cancelColorDropper()
    {
        if (mColorPicker != nullptr)
        {
            mColorPicker->cancelEyeDropper();
        }
    }

    void RoseObjectEditor::closeValueEdit()
    {
        mEdit = {};
        this->notifyRebuild();
    }

    void RoseObjectEditor::beginValueEdit(ValueEditState::Kind kind, u32_t column, const std::vector<NestedHop>& path, const NestedField& info, const String& shown)
    {
        mEdit.kind = kind;
        mEdit.column = column;
        mEdit.path = path;
        mEdit.fieldKind = info.kind;
        mEdit.options = info.options;
        if (kind == ValueEditState::Kind::Date)
        {
            initTemporalParts(info.kind, shown, mEdit.date);
        }
        this->notifyRebuild();
    }

    bool RoseObjectEditor::editingField(ValueEditState::Kind kind, u32_t column, const std::vector<NestedHop>& path) const
    {
        return mEdit.kind == kind && mEdit.column == column && samePath(mEdit.path, path);
    }

    void RoseObjectEditor::commitText(u32_t column, const std::vector<NestedHop>& path, const String& text)
    {
        if (mDoc == nullptr || !mDoc->isOpen() || mRow < 0)
        {
            return;
        }
        if (path.empty())
        {
            if (mDoc->cellText(static_cast<u32_t>(mRow), column) == text)
            {
                return;
            }
            mDoc->setCellText(static_cast<u32_t>(mRow), column, text);
        }
        else
        {
            if (mDoc->nestedText(static_cast<u32_t>(mRow), column, path) == text)
            {
                return;
            }
            mDoc->setNestedText(static_cast<u32_t>(mRow), column, path, text);
        }
        this->notifyRebuild();
    }

    void RoseObjectEditor::commitBool(u32_t column, const std::vector<NestedHop>& path, bool value)
    {
        this->commitText(column, path, value ? "true" : "false");
    }

    void RoseObjectEditor::commitLabels(u32_t column, const std::vector<NestedHop>& path, const std::vector<String>& labels)
    {
        if (mDoc == nullptr || !mDoc->isOpen() || mRow < 0)
        {
            return;
        }
        if (path.empty())
        {
            mDoc->setCellLabels(static_cast<u32_t>(mRow), column, labels);
        }
        else
        {
            mDoc->setNestedLabels(static_cast<u32_t>(mRow), column, path, labels);
        }
        this->notifyRebuild();
    }

    void RoseObjectEditor::rebuild()
    {
        this->clearOverlayEditors();
        this->detachChildren();
        mView = nullptr;
        mFocus = nullptr;
        if (mDoc == nullptr || !mDoc->isOpen() || !mDoc->hasActiveTable() || mRow < 0)
        {
            return;
        }
        if (static_cast<u32_t>(mRow) >= mDoc->rowCount())
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
        this->color = t.raised;
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

        auto view = std::make_shared<UIView>();
        view->shape.pivot = Vector2(0.0f, 0.0f);
        view->color = t.raised;
        setBox(view, Rect(0.0f, 0.0f, panelW, shape.size.y > 1.0f ? shape.size.y : 400.0f * s));

        auto body = std::make_shared<UIList>();
        body->direction = UIDirection::Vertical;
        body->padding = 0.0f;
        body->spacing = 0.0f;
        body->color = clear;
        body->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(body, Rect(0.0f, 0.0f, panelW, 0.0f));

        auto head = std::make_shared<UIWidget>();
        head->pickable = false;
        head->shape.pivot = Vector2(0.0f, 0.0f);
        head->color = t.header;
        setBox(head, Rect(0.0f, 0.0f, panelW, headH));
        float titleW = atLeast(panelW - headPad * 2.0f - closeW - 8.0f * s, 80.0f * s);
        auto title = makeText("对象属性", fontPath, fontPx, ink, titleW, 20.0f * s);
        title->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(title, Rect(headPad, 4.0f * s, titleW, 20.0f * s));
        head->attachChild(title);
        String tableName = mDoc->tableName(mDoc->activeTableIndex());
        String sub = String::format("%s · 第 %d 行", tableName.cstr(), mRow + 1);
        if (mColumn >= 0 && static_cast<u32_t>(mColumn) < mDoc->columnCount())
        {
            sub += " · ";
            sub += mDoc->column(static_cast<u32_t>(mColumn)).name;
        }
        auto subtitle = makeText(sub, fontPath, smallPx, mute, titleW, 16.0f * s);
        subtitle->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(subtitle, Rect(headPad, 24.0f * s, titleW, 16.0f * s));
        head->attachChild(subtitle);
        auto close = makeButton("关闭", fontPath, smallPx, closeW, 26.0f * s, quiet, ink);
        close->shape.setBox(Vector2(panelW - headPad - closeW, 9.0f * s), Vector2(closeW, 26.0f * s));
        close->onClick = [this]()
        {
            mRow = -1;
            mColumn = -1;
            mEdit = {};
            this->notifyRebuild();
        };
        head->attachChild(close);
        auto headRule = std::make_shared<UIWidget>();
        headRule->pickable = false;
        headRule->shape.pivot = Vector2(0.0f, 0.0f);
        headRule->color = grid;
        setBox(headRule, Rect(0.0f, headH - 1.0f, panelW, 1.0f));
        head->attachChild(headRule);
        body->attachChild(head);

        const u32_t cols = mDoc->columnCount();
        if (cols == 0)
        {
            auto empty = makeText("没有字段", fontPath, fontPx, mute, panelW - headPad * 2.0f, 22.0f * s);
            empty->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(empty, Rect(headPad, 0.0f, panelW - headPad * 2.0f, 22.0f * s));
            body->attachChild(empty);
            view->addChild(body);
            this->attachChild(view);
            mView = view.get();
            view->setScroll(0.0f, mKeepY);
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
        tree->color = t.raised;
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
            String key = foldKey(col, path);
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
                toggle->onValueChanged = [this, col, path](bool value) { this->commitBool(col, path, value); };
                hit->attachChild(toggle);
                if (mFocus == nullptr)
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
                        ? mDoc->cellLabels(static_cast<u32_t>(mRow), col)
                        : mDoc->nestedLabels(static_cast<u32_t>(mRow), col, path);
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
                            this->commitText(col, path, String::empty);
                            return;
                        }
                        std::vector<String> current = path.empty()
                            ? mDoc->cellLabels(static_cast<u32_t>(mRow), col)
                            : mDoc->nestedLabels(static_cast<u32_t>(mRow), col, path);
                        this->commitLabels(col, path, removeLabel(current, label));
                    },
                    [this, col, path, info, shown]()
                    {
                        this->beginValueEdit(ValueEditState::Kind::Option, col, path, info, shown);
                    });
                hit->onClick = [this, col, path, info, shown]()
                {
                    this->beginValueEdit(ValueEditState::Kind::Option, col, path, info, shown);
                };
                if (this->editingField(ValueEditState::Kind::Option, col, path))
                {
                    mPickAnchor = hit.get();
                }
                if (mFocus == nullptr)
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
                    this->beginValueEdit(ValueEditState::Kind::Date, col, path, info, shown);
                };
                if (this->editingField(ValueEditState::Kind::Date, col, path))
                {
                    mDateAnchor = hit.get();
                }
                if (mFocus == nullptr)
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
                    this->beginValueEdit(ValueEditState::Kind::Color, col, path, info, shown);
                };
                if (this->editingField(ValueEditState::Kind::Color, col, path))
                {
                    mColorAnchor = hit.get();
                }
                if (mFocus == nullptr)
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
                        this->commitText(col, path, picked);
                    }
                };
                if (mFocus == nullptr)
                {
                    mFocus = hit.get();
                }
                return hit;
            }

            auto input = std::make_shared<UIInput>();
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
            input->onFocusLoss = [this, raw, col, path]()
            {
                if (raw != nullptr)
                {
                    this->commitText(col, path, raw->text);
                }
            };
            input->onSubmit = [this, col, path](const String& text) { this->commitText(col, path, text); };
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
            if (mFocus == nullptr)
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
            auto del = makeDangerButton("删", fontPath, smallPx, delW, valueInnerH);
            fitRowButton(del, valueInnerH);
            u32_t itemIndex = path.back().index;
            del->onClick = [this, col, path, itemIndex]()
            {
                std::vector<NestedHop> listPath(path.begin(), path.end() - 1);
                mDoc->nestedListRemove(static_cast<u32_t>(mRow), col, listPath, itemIndex);
                this->notifyRebuild();
            };
            row->attachChild(del);
            return row;
        };

        std::function<void(UITableRow*, u32_t, const std::vector<NestedHop>&, const NestedField&)> attachField;
        attachField = [&](UITableRow* parent, u32_t col, const std::vector<NestedHop>& path, const NestedField& info)
        {
            String shown = path.empty()
                ? mDoc->cellText(static_cast<u32_t>(mRow), col)
                : mDoc->nestedText(static_cast<u32_t>(mRow), col, path);
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
                u32_t members = mDoc->nestedFieldCount(col, path);
                if (members == 0)
                {
                    UITableRow* empty = line->addRow(treeRowH);
                    paintLine(empty, false);
                    bindName(empty, "没有成员", true);
                }
                for (u32_t index = 0; index < members; ++index)
                {
                    NestedField child = mDoc->nestedField(col, path, index);
                    std::vector<NestedHop> childPath = path;
                    childPath.push_back(NestedHop::field(child.id));
                    attachField(line, col, childPath, child);
                }
                return;
            }
            if (info.nestedList && path.size() < 8)
            {
                bindFold(line, col, path);
                auto add = makeButton("添加一项", fontPath, smallPx, 96.0f * s, valueInnerH, quiet, ink);
                fitRowButton(add, valueInnerH);
                add->onClick = [this, col, path]()
                {
                    u32_t count = mDoc->nestedListCount(static_cast<u32_t>(mRow), col, path);
                    mDoc->nestedListInsert(static_cast<u32_t>(mRow), col, path, count);
                    this->notifyRebuild();
                };
                UIWidget::Ref value = add;
                if (listItem)
                {
                    value = wrapDelete(add, col, path, valueInnerW);
                }
                line->setContent(1, value);
                u32_t count = mDoc->nestedListCount(static_cast<u32_t>(mRow), col, path);
                if (count == 0)
                {
                    UITableRow* empty = line->addRow(treeRowH);
                    paintLine(empty, false);
                    bindName(empty, "没有项目", true);
                }
                for (u32_t index = 0; index < count; ++index)
                {
                    NestedField item = mDoc->nestedListElement(col, path);
                    item.name = String::format("第 %u 项", index + 1);
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

        if (mColumn >= 0)
        {
            if (static_cast<u32_t>(mColumn) < cols && mDoc->isNestedStructColumn(static_cast<u32_t>(mColumn)))
            {
                u32_t col = static_cast<u32_t>(mColumn);
                u32_t members = mDoc->nestedFieldCount(col, {});
                if (members == 0)
                {
                    UITableRow* empty = tree->addRow(treeRowH);
                    paintLine(empty, false);
                    bindName(empty, "没有成员", true);
                }
                for (u32_t index = 0; index < members; ++index)
                {
                    NestedField child = mDoc->nestedField(col, {}, index);
                    attachField(nullptr, col, { NestedHop::field(child.id) }, child);
                }
            }
        }
        else
        {
            for (u32_t col = 0; col < cols; ++col)
            {
                attachField(nullptr, col, {}, mDoc->objectField(col));
            }
        }

        setBox(tree, Rect(0.0f, 0.0f, panelW, 0.0f));
        tree->relayout();
        body->attachChild(tree);
        view->addChild(body);
        this->attachChild(view);
        mView = view.get();
        view->setScroll(0.0f, mKeepY);

        if (mHost.overlay != nullptr && mEdit.kind == ValueEditState::Kind::Option && mPickAnchor != nullptr)
        {
            std::vector<String> selected = mEdit.path.empty()
                ? mDoc->cellLabels(static_cast<u32_t>(mRow), mEdit.column)
                : mDoc->nestedLabels(static_cast<u32_t>(mRow), mEdit.column, mEdit.path);
            u32_t column = mEdit.column;
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
                [this, column, path, kind](const String& label)
                {
                    if (kind == FieldKind::MultiOption)
                    {
                        std::vector<String> current = path.empty()
                            ? mDoc->cellLabels(static_cast<u32_t>(mRow), column)
                            : mDoc->nestedLabels(static_cast<u32_t>(mRow), column, path);
                        if (selectedContains(current, label))
                        {
                            this->commitLabels(column, path, removeLabel(current, label));
                        }
                        else
                        {
                            current.push_back(label);
                            this->commitLabels(column, path, current);
                        }
                    }
                    else
                    {
                        mEdit = {};
                        this->commitText(column, path, label);
                    }
                },
                [this]() { this->closeValueEdit(); });
            mHost.overlay->attachChild(menu);
            mPickMenu = menu.get();
            mFocus = menu.get();
        }

        if (mHost.overlay != nullptr && mEdit.kind == ValueEditState::Kind::Date && mDateAnchor != nullptr)
        {
            FieldKind dateKind = mEdit.fieldKind;
            u32_t column = mEdit.column;
            std::vector<NestedHop> path = mEdit.path;
            DatePanelWidgets widgets = makeDatePanel(
                dateKind,
                mEdit.date,
                s,
                fontPath,
                fontPx,
                smallPx,
                [this]() { this->closeValueEdit(); },
                [this, column, path]()
                {
                    mEdit = {};
                    this->commitText(column, path, String::empty);
                },
                [this, column, path](const String& stored)
                {
                    mEdit = {};
                    this->commitText(column, path, stored);
                },
                [this]() { this->handleEscape(); },
                [this](const TemporalParts& next) { mEdit.date = next; });
            mHost.overlay->attachChild(widgets.panel);
            mDatePanel = widgets.panel.get();
            mFocus = widgets.focus;
        }

        if (mHost.overlay != nullptr && mEdit.kind == ValueEditState::Kind::Color && mColorAnchor != nullptr)
        {
            Color current(1.0f, 1.0f, 1.0f, 1.0f);
            String shown = mEdit.path.empty()
                ? mDoc->cellText(static_cast<u32_t>(mRow), mEdit.column)
                : mDoc->nestedText(static_cast<u32_t>(mRow), mEdit.column, mEdit.path);
            parseColor(shown, current);
            u32_t column = mEdit.column;
            std::vector<NestedHop> path = mEdit.path;
            ColorPanelWidgets widgets = makeColorPanel(
                current,
                s,
                fontPath,
                fontPx,
                smallPx,
                mHost.writeClipboard,
                mHost.readClipboard,
                [this, column, path](const Color& value)
                {
                    mEdit = {};
                    this->commitText(column, path, formatColor(value));
                },
                [this, column, path]()
                {
                    mEdit = {};
                    this->commitText(column, path, String::empty);
                },
                [this]() { this->closeValueEdit(); },
                [this]() { this->handleEscape(); });
            mHost.overlay->attachChild(widgets.panel);
            mColorPanel = widgets.panel.get();
            mColorPicker = widgets.picker;
            mFocus = widgets.focus;
        }
    }

}
