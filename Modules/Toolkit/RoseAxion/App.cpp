#include "./App.h"

#include "Base/Time.h"
#include "UI/Main.h"
#include "UI/UIFont.h"
#include "UI/UIWindow.h"

#include <utility>
#include <vector>

namespace eokas::RoseAxion {

    namespace {

        const char* kPresets[] = {
            "#ffffff", "#e5e7eb", "#9ca3af", "#374151", "#111827",
            "#fecaca", "#ef4444", "#b91c1c",
            "#fed7aa", "#f97316", "#c2410c",
            "#fef08a", "#eab308", "#a16207",
            "#bbf7d0", "#22c55e", "#15803d",
            "#99f6e4", "#14b8a6", "#0f766e",
            "#bfdbfe", "#3b82f6", "#1d4ed8",
            "#c7d2fe", "#6366f1", "#4338ca",
            "#f5d0fe", "#d946ef", "#a21caf",
        };

        const FieldKind kKinds[] = {
            FieldKind::Text,
            FieldKind::Integer,
            FieldKind::Float,
            FieldKind::Boolean,
            FieldKind::DateTime,
            FieldKind::Url,
            FieldKind::Email,
            FieldKind::SingleOption,
        };

        int clampi(int value, int lo, int hi)
        {
            if (value < lo) return lo;
            if (value > hi) return hi;
            return value;
        }

        void setBox(const std::shared_ptr<UIWidget>& widget, const Rect& box)
        {
            widget->shape.size = box.size;
            widget->shape.origin = box.origin + widget->shape.pivot * widget->shape.size;
        }

        void styleText(UIText& text, const String& font, float size, const Color& color)
        {
            text.style.fontPath = font;
            text.style.fontSize = size;
            text.style.color = color;
        }

        bool colorFromHex(const String& text, Color& out)
        {
            String value = trimCopy(text);
            if (!value.isEmpty() && value.at(0) == '#')
            {
                value = value.substr(1);
            }
            if (value.length() != 6)
            {
                return false;
            }
            auto hex = [](char c) -> int
            {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            int channels[3] = {};
            for (int i = 0; i < 3; ++i)
            {
                int hi = hex(value.at(static_cast<size_t>(i * 2)));
                int lo = hex(value.at(static_cast<size_t>(i * 2 + 1)));
                if (hi < 0 || lo < 0)
                {
                    return false;
                }
                channels[i] = hi * 16 + lo;
            }
            out = Color(channels[0] / 255.0f, channels[1] / 255.0f, channels[2] / 255.0f, 1.0f);
            return true;
        }

        std::shared_ptr<UIText> makeText(const String& value, const String& font, float size, const Color& color, float width, float height)
        {
            auto text = std::make_shared<UIText>();
            text->text = value;
            styleText(*text, font, size, color);
            text->maxWidth = width;
            setBox(text, Rect(0.0f, 0.0f, width, height));
            return text;
        }

        std::shared_ptr<UIButton> makeButton(const String& label, const String& font, float size, float width, float height, const Color& fill, const Color& ink)
        {
            auto button = std::make_shared<UIButton>();
            button->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(button, Rect(0.0f, 0.0f, width, height));
            button->paddingX = 8.0f;
            button->paddingY = 4.0f;
            button->background = fill;
            button->hoverFill = Color(fill.r + 0.08f, fill.g + 0.08f, fill.b + 0.08f, 1.0f);
            button->pressedFill = Color(fill.r * 0.8f, fill.g * 0.8f, fill.b * 0.8f, 1.0f);
            button->setText(label);
            if (UIText* text = button->label())
            {
                styleText(*text, font, size, ink);
            }
            return button;
        }

        std::shared_ptr<UIButton> makeMoreButton(const String& font, float size, float width, float height, const Color& ink)
        {
            auto button = makeButton("...", font, size, width, height, Color(0.0f, 0.0f, 0.0f, 0.0f), ink);
            button->paddingX = 0.0f;
            button->paddingY = 0.0f;
            button->hoverFill = Color(1.0f, 1.0f, 1.0f, 0.10f);
            button->pressedFill = Color(0.0f, 0.0f, 0.0f, 0.16f);
            return button;
        }

        struct EscapeInput : UIInput
        {
            std::function<void()> onEscape;
            bool handleKeyPress(const UIKeyEvent& event) override
            {
                if (event.key == UIKeyCode::Escape)
                {
                    if (onEscape)
                    {
                        onEscape();
                    }
                    return true;
                }
                return UIInput::handleKeyPress(event);
            }
        };

        UIView* findView(UIWidget* node)
        {
            if (node == nullptr)
            {
                return nullptr;
            }
            if (UIView* view = dynamic_cast<UIView*>(node))
            {
                return view;
            }
            for (const auto& child : node->children())
            {
                if (UIView* view = findView(child.get()))
                {
                    return view;
                }
            }
            return nullptr;
        }

        bool statusFailed(const String& text)
        {
            return text == "内容与字段类型不符，已清空"
                || text == "无法打开文件"
                || text == "无法保存文件"
                || text == "打开失败"
                || text == "这不是 RoseAxion 表格"
                || text == "保存失败"
                || text == "颜色需要是 #rrggbb"
                || text == "日期时间格式无效";
        }

        void revealCell(UIView& view, int row, int col, float headW, float colW, float rowH)
        {
            if (row < 0 || col < 0)
            {
                return;
            }
            float x = headW + static_cast<float>(col) * colW;
            float y = rowH + static_cast<float>(row) * rowH;
            Rect vp = view.viewport();
            float dx = 0.0f;
            float dy = 0.0f;
            if (x < view.scrollX())
            {
                dx = x - view.scrollX();
            }
            else if (x + colW > view.scrollX() + vp.size.x)
            {
                dx = x + colW - (view.scrollX() + vp.size.x);
            }
            if (y < view.scrollY())
            {
                dy = y - view.scrollY();
            }
            else if (y + rowH > view.scrollY() + vp.size.y)
            {
                dy = y + rowH - (view.scrollY() + vp.size.y);
            }
            if (dx != 0.0f || dy != 0.0f)
            {
                view.scrollBy(dx, dy);
            }
        }

        struct MenuGate
        {
            UIMenu* file = nullptr;
            UIWidget* extra = nullptr;
            UIWidget* dismiss = nullptr;
            void hide()
            {
                if (file != nullptr) file->visible = false;
                if (extra != nullptr) extra->visible = false;
                if (dismiss != nullptr) dismiss->visible = false;
            }
        };

    }

    void AxionApp::attach(UIWindow& window, const AxionHost& host, const String& fontPath, float scale)
    {
        mWindow = &window;
        mHost = host;
        mFont = fontPath;
        mScale = scale < 0.5f ? 1.0f : scale;
        mRebuild = true;
    }

    void AxionApp::resize(float width, float height)
    {
        if (width < 1.0f) width = 1.0f;
        if (height < 1.0f) height = 1.0f;
        mWidth = width;
        mHeight = height;
        this->requestRebuild();
    }

    void AxionApp::tick()
    {
        if (!mRebuild || mWindow == nullptr)
        {
            return;
        }
        if (mWindow->pressed() != nullptr)
        {
            return;
        }
        mRebuild = false;
        this->build();
    }

    void AxionApp::requestRebuild()
    {
        mRebuild = true;
    }

    String AxionApp::fileTitle() const
    {
        const String& path = mDoc.path();
        if (path.isEmpty())
        {
            return "未命名";
        }
        size_t slash = path.rfind('\\');
        size_t slash2 = path.rfind('/');
        size_t pos = String::npos;
        if (slash != String::npos) pos = slash;
        if (slash2 != String::npos && (pos == String::npos || slash2 > pos)) pos = slash2;
        if (pos == String::npos)
        {
            return path;
        }
        return path.substr(pos + 1);
    }

    void AxionApp::notifyTitle()
    {
        if (!mHost.setTitle)
        {
            return;
        }
        String title = "RoseAxion";
        if (mDoc.isOpen())
        {
            title = title + " - " + this->fileTitle();
            if (mDoc.dirty())
            {
                title = title + " *";
            }
        }
        mHost.setTitle(title);
    }

    bool AxionApp::handleShortcut(UIKeyCode key, bool ctrl)
    {
        if (!ctrl)
        {
            return false;
        }
        if (key == UIKeyCode::N)
        {
            this->newDocument();
            return true;
        }
        if (key == UIKeyCode::O)
        {
            this->openDocument();
            return true;
        }
        if (key == UIKeyCode::S)
        {
            this->saveDocument();
            return true;
        }
        return false;
    }

    void AxionApp::newDocument()
    {
        mDoc.createNew();
        mSelect = SelectKind::Cell;
        mSelectRow = 0;
        mSelectCol = 0;
        mEdit = EditKind::None;
        mSettingsCol = -1;
        mDateRow = -1;
        mDateCol = -1;
        mPickRow = -1;
        mPickCol = -1;
        mColorIndex = -1;
        mConfirm = ConfirmKind::None;
        mResetScroll = true;
        this->requestRebuild();
    }

    void AxionApp::openDocument()
    {
        if (!mHost.pickOpenPath)
        {
            mDoc.setStatus("无法打开文件");
            this->requestRebuild();
            return;
        }
        String path = mHost.pickOpenPath();
        if (path.isEmpty())
        {
            return;
        }
        if (mDoc.load(path))
        {
            mSelect = SelectKind::Cell;
            mSelectRow = 0;
            mSelectCol = 0;
            mEdit = EditKind::None;
            mSettingsCol = -1;
            mDateRow = -1;
            mDateCol = -1;
            mPickRow = -1;
            mPickCol = -1;
            mColorIndex = -1;
            mConfirm = ConfirmKind::None;
            mResetScroll = true;
        }
        this->requestRebuild();
    }

    void AxionApp::saveDocument()
    {
        if (!mDoc.isOpen())
        {
            return;
        }
        if (!mHost.pickSavePath)
        {
            mDoc.setStatus("无法保存文件");
            this->requestRebuild();
            return;
        }
        String suggested = mDoc.path().isEmpty() ? String("table.rose") : mDoc.path();
        String path = mHost.pickSavePath(suggested);
        if (path.isEmpty())
        {
            return;
        }
        mDoc.save(path);
        this->requestRebuild();
    }

    void AxionApp::finishEdit(const String& text, int rowDelta, int colDelta)
    {
        if (mEdit == EditKind::None || !mDoc.isOpen())
        {
            return;
        }
        if (mEdit == EditKind::Header)
        {
            mDoc.setColumnName(static_cast<u32_t>(mEditCol), text);
            mSelect = SelectKind::Cell;
            mSelectRow = 0;
            mSelectCol = mEditCol;
        }
        else
        {
            mDoc.setCellText(static_cast<u32_t>(mEditRow), static_cast<u32_t>(mEditCol), text);
            mSelect = SelectKind::Cell;
            mSelectRow = clampi(mSelectRow + rowDelta, 0, static_cast<int>(mDoc.rowCount()) - 1);
            mSelectCol = clampi(mSelectCol + colDelta, 0, static_cast<int>(mDoc.columnCount()) - 1);
        }
        mEdit = EditKind::None;
        this->requestRebuild();
    }

    void AxionApp::beginEdit(int row, int col, const String& seed)
    {
        if (!mDoc.isOpen())
        {
            return;
        }
        FieldKind kind = mDoc.column(static_cast<u32_t>(col)).kind;
        mSelect = SelectKind::Cell;
        mSelectRow = row;
        mSelectCol = col;
        mEdit = EditKind::None;
        if (kind == FieldKind::DateTime)
        {
            String shown = mDoc.cellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
            size_t space = shown.find(' ');
            if (space != String::npos)
            {
                String date = shown.substr(0, space);
                String time = shown.substr(space + 1);
                mDateDraft = String::empty;
                for (size_t i = 0; i < date.length(); ++i)
                {
                    char c = date.at(i);
                    mDateDraft = mDateDraft + String(c == '/' ? '-' : c);
                }
                mTimeDraft = time;
            }
            else
            {
                TimePoint now;
                mDateDraft = now.toString("YYYY-MM-DD");
                mTimeDraft = now.toString("hh:mm:ss");
            }
            mDateRow = row;
            mDateCol = col;
            mSettingsCol = -1;
            mPickRow = -1;
            mPickCol = -1;
            this->requestRebuild();
            return;
        }
        if (kind == FieldKind::SingleOption)
        {
            mPickRow = row;
            mPickCol = col;
            mSettingsCol = -1;
            this->requestRebuild();
            return;
        }
        mEdit = EditKind::Cell;
        mEditRow = row;
        mEditCol = col;
        mEditSeed = seed;
        mPickRow = -1;
        mPickCol = -1;
        this->requestRebuild();
    }

    void AxionApp::selectCell(int row, int col)
    {
        auto now = std::chrono::steady_clock::now();
        bool repeat = !mClickHeader && mClickRow == row && mClickCol == col && now - mClickTime < std::chrono::milliseconds(450);
        mClickTime = now;
        mClickRow = row;
        mClickCol = col;
        mClickHeader = false;
        mSelect = SelectKind::Cell;
        mSelectRow = row;
        mSelectCol = col;
        if (repeat)
        {
            this->beginEdit(row, col, mDoc.cellText(static_cast<u32_t>(row), static_cast<u32_t>(col)));
            return;
        }
        mPickRow = -1;
        mPickCol = -1;
        this->requestRebuild();
    }

    void AxionApp::selectAll()
    {
        if (!mDoc.isOpen())
        {
            return;
        }
        mSelect = SelectKind::All;
        mPickRow = -1;
        mPickCol = -1;
        mClickHeader = false;
        mClickRow = -1;
        mClickCol = -1;
        this->requestRebuild();
    }

    bool AxionApp::handleEscape()
    {
        if (mConfirm != ConfirmKind::None)
        {
            mConfirm = ConfirmKind::None;
            this->requestRebuild();
            return true;
        }
        if (mDateRow >= 0 || mDateCol >= 0)
        {
            mDateRow = -1;
            mDateCol = -1;
            this->requestRebuild();
            return true;
        }
        if (mEdit != EditKind::None)
        {
            mEdit = EditKind::None;
            this->requestRebuild();
            return true;
        }
        return false;
    }

    void AxionApp::build()
    {
        if (mWindow == nullptr)
        {
            return;
        }
        float keepX = 0.0f;
        float keepY = 0.0f;
        if (!mResetScroll)
        {
            if (UIView* old = findView(mWindow->root().get()))
            {
                keepX = old->scrollX();
                keepY = old->scrollY();
            }
        }
        mResetScroll = false;

        UIWidget* focus = nullptr;
        const float s = mScale;
        const float fontPx = 15.0f * s;
        const float smallPx = 13.0f * s;
        const float barH = 52.0f * s;
        const float rowH = 36.0f * s;
        const float colW = 168.0f * s;
        const float headW = 84.0f * s;
        const float drawerW = 360.0f * s;
        const Color canvas(0.102f, 0.110f, 0.133f, 1.0f);
        const Color bar(0.078f, 0.086f, 0.110f, 1.0f);
        const Color paper(0.145f, 0.157f, 0.196f, 1.0f);
        const Color headerBg(0.196f, 0.216f, 0.267f, 1.0f);
        const Color selected(0.275f, 0.400f, 0.620f, 1.0f);
        const Color ink(0.925f, 0.933f, 0.953f, 1.0f);
        const Color mute(0.620f, 0.660f, 0.720f, 1.0f);
        const Color accent(0.345f, 0.545f, 0.930f, 1.0f);
        const Color field(0.110f, 0.118f, 0.149f, 1.0f);
        const Color popup(0.160f, 0.176f, 0.220f, 1.0f);
        const Color url(0.455f, 0.710f, 0.980f, 1.0f);
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const Color warn(0.96f, 0.72f, 0.42f, 1.0f);

        if (mDoc.isOpen())
        {
            int rows = static_cast<int>(mDoc.rowCount());
            int cols = static_cast<int>(mDoc.columnCount());
            mSelectRow = clampi(mSelectRow, 0, rows - 1);
            mSelectCol = clampi(mSelectCol, 0, cols - 1);
            if (mSelect == SelectKind::None) mSelect = SelectKind::Cell;
            if (mSettingsCol >= cols) mSettingsCol = -1;
            if (mDateRow >= rows || mDateCol >= cols) { mDateRow = -1; mDateCol = -1; }
            if (mPickRow >= rows || mPickCol >= cols) { mPickRow = -1; mPickCol = -1; }
            if (mEdit == EditKind::Cell && (mEditRow >= rows || mEditCol >= cols)) mEdit = EditKind::None;
            if (mEdit == EditKind::Header && mEditCol >= cols) mEdit = EditKind::None;
        }

        auto root = std::make_shared<UIWidget>();
        root->shape.pivot = Vector2(0.0f, 0.0f);
        root->pickable = false;
        setBox(root, Rect(0.0f, 0.0f, mWidth, mHeight));
        root->color = canvas;
        root->focusable = true;
        root->onKeyPress = [this](const UIKeyEvent& event)
        {
            if (event.key == UIKeyCode::Escape)
            {
                this->handleEscape();
                return;
            }
            this->handleShortcut(event.key, event.ctrl);
        };

        auto menuBar = std::make_shared<UIMenu>();
        menuBar->direction = UIDirection::Horizontal;
        menuBar->padding = 2.0f * s;
        menuBar->spacing = 0.0f;
        menuBar->color = bar;
        menuBar->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(menuBar, Rect(0.0f, 0.0f, mWidth, 0.0f));

        auto fileMenu = std::make_shared<UIMenu>();
        fileMenu->direction = UIDirection::Vertical;
        fileMenu->padding = 4.0f * s;
        fileMenu->spacing = 2.0f * s;
        fileMenu->color = popup;
        fileMenu->overlay = true;
        fileMenu->visible = false;
        fileMenu->shape.pivot = Vector2(0.0f, 0.0f);

        auto dismiss = std::make_shared<UIWidget>();
        dismiss->overlay = true;
        dismiss->visible = false;
        dismiss->pickable = true;
        dismiss->color = clear;
        dismiss->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(dismiss, Rect(0.0f, 0.0f, mWidth, mHeight));
        auto gate = std::make_shared<MenuGate>();
        gate->file = fileMenu.get();
        gate->dismiss = dismiss.get();
        dismiss->onClick = [gate]() { gate->hide(); };

        auto addMenuItem = [&](UIMenu& menu, const char* label, const std::function<void()>& action)
        {
            auto item = std::make_shared<UIMenuItem>();
            item->paddingX = 14.0f * s;
            item->paddingY = 8.0f * s;
            item->background = popup;
            item->hoverFill = selected;
            item->setText(label);
            if (UIText* text = item->label())
            {
                styleText(*text, mFont, fontPx, ink);
            }
            item->onClick = [action, gate]()
            {
                gate->hide();
                action();
            };
            menu.addItem(item);
            return item;
        };
        auto fileItem = std::make_shared<UIMenuItem>();
        fileItem->paddingX = 16.0f * s;
        fileItem->paddingY = 8.0f * s;
        fileItem->background = bar;
        fileItem->hoverFill = Color(0.196f, 0.275f, 0.412f, 1.0f);
        fileItem->setText("文件");
        if (UIText* text = fileItem->label())
        {
            styleText(*text, mFont, fontPx, ink);
        }
        menuBar->addItem(fileItem);
        fileMenu->host = fileItem.get();
        menuBar->layout();
        float menuH = menuBar->shape.size.y;
        if (menuH < 28.0f * s)
        {
            menuH = 28.0f * s;
        }
        setBox(menuBar, Rect(0.0f, 0.0f, mWidth, menuH));
        UIMenuItem* fileRaw = fileItem.get();
        UIMenu* fileMenuRaw = fileMenu.get();
        UIWidget* rootRawForMenu = root.get();
        fileItem->onClick = [fileRaw, fileMenuRaw, rootRawForMenu, gate, s]()
        {
            bool open = !fileMenuRaw->visible;
            gate->hide();
            if (open)
            {
                fileMenuRaw->visible = true;
                if (gate->dismiss != nullptr) gate->dismiss->visible = true;
                Rect bounds = fileRaw->screenBounds();
                Vector2 local = rootRawForMenu->screenToLocal(Vector2(bounds.origin.x, bounds.origin.y + bounds.size.y));
                fileMenuRaw->shape.pivot = Vector2(0.0f, 0.0f);
                fileMenuRaw->shape.setBox(local, Vector2(160.0f * s, 36.0f * s * 3.0f + 12.0f * s));
            }
        };
        addMenuItem(*fileMenu, "新建", [this]() { this->newDocument(); });
        addMenuItem(*fileMenu, "打开...", [this]() { this->openDocument(); });
        addMenuItem(*fileMenu, "保存...", [this]() { this->saveDocument(); });

        auto toolbar = std::make_shared<UIList>();
        toolbar->direction = UIDirection::Horizontal;
        toolbar->padding = 8.0f * s;
        toolbar->spacing = 8.0f * s;
        toolbar->color = bar;
        toolbar->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(toolbar, Rect(0.0f, menuH, mWidth, barH));

        auto addTool = [&](const char* label, const std::function<void()>& action)
        {
            auto button = makeButton(label, mFont, fontPx, 120.0f * s, 32.0f * s, accent, ink);
            button->onClick = action;
            toolbar->attachChild(button);
        };
        addTool("新建表格", [this]() { this->newDocument(); });
        addTool("打开 Library", [this]() { this->openDocument(); });
        addTool("保存 Library", [this]() { this->saveDocument(); });
        const float toolW = 120.0f * s;
        const float toolPad = 8.0f * s;
        const float toolGap = 8.0f * s;
        float statusW = mWidth - (toolPad * 2.0f + toolW * 3.0f + toolGap * 3.0f);
        if (statusW < 80.0f * s) statusW = 80.0f * s;
        String statusText = mDoc.status();
        Color statusColor = statusFailed(statusText) ? warn : mute;
        if (statusText.isEmpty()) statusText = String(" ");
        auto status = makeText(statusText, mFont, smallPx, statusColor, statusW, 24.0f * s);
        toolbar->attachChild(status);
        root->attachChild(menuBar);
        root->attachChild(toolbar);
        root->attachChild(dismiss);
        root->attachChild(fileMenu);

        const float chromeH = menuH + barH;
        float drawer = (mDoc.isOpen() && (mSettingsCol >= 0 || mDateRow >= 0)) ? drawerW : 0.0f;
        float gridW = mWidth - drawer;
        if (gridW < 80.0f) gridW = 80.0f;
        float gridH = mHeight - chromeH;
        if (gridH < 40.0f) gridH = 40.0f;

        if (!mDoc.isOpen())
        {
            auto empty = makeText("点击「打开 Library」或「新建表格」开始", mFont, fontPx + 2.0f, mute, 640.0f * s, 32.0f * s);
            empty->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(empty, Rect(48.0f * s, chromeH + 80.0f * s, 640.0f * s, 32.0f * s));
            root->attachChild(empty);
            focus = root.get();
        }
        else
        {
            int rows = static_cast<int>(mDoc.rowCount());
            int cols = static_cast<int>(mDoc.columnCount());
            auto view = std::make_shared<UIView>();
            view->shape.pivot = Vector2(0.0f, 0.0f);
            view->color = canvas;
            setBox(view, Rect(0.0f, chromeH, gridW, gridH));

            auto table = std::make_shared<UITable>();
            table->shape.pivot = Vector2(0.0f, 0.0f);
            const Color gridLine(0.30f, 0.33f, 0.40f, 1.0f);
            table->border = UITableBorder(1.0f, 1.0f, 0.0f, 0.0f, gridLine);
            table->cellPadding = 0.0f;
            table->cellSpacing = 0.0f;
            table->color = paper;
            table->addColumn(headW);
            for (int col = 0; col < cols; ++col)
            {
                table->addColumn(colW);
            }
            for (int row = 0; row < rows + 1; ++row)
            {
                table->addRow(rowH);
            }
            float tableW = headW + colW * static_cast<float>(cols);
            float tableH = rowH * static_cast<float>(rows + 1);
            setBox(table, Rect(0.0f, 0.0f, tableW, tableH));
            auto paintCell = [&](UITableCell* cell, int tableRow, int tableCol, const Color& fill)
            {
                if (cell == nullptr)
                {
                    return;
                }
                cell->color = fill;
                bool lastCol = tableCol == cols;
                bool lastRow = tableRow == rows;
                cell->setBorder(UITableBorder(1.0f, 1.0f, lastCol ? 1.0f : 0.0f, lastRow ? 1.0f : 0.0f, gridLine));
            };
            auto innerBox = [&](int tableRow, int tableCol, float spanW, float spanH)
            {
                bool lastCol = tableCol == cols;
                bool lastRow = tableRow == rows;
                float w = spanW - 1.0f - (lastCol ? 1.0f : 0.0f);
                float h = spanH - 1.0f - (lastRow ? 1.0f : 0.0f);
                if (w < 0.0f) w = 0.0f;
                if (h < 0.0f) h = 0.0f;
                return Vector2(w, h);
            };
            const float textPadX = 6.0f * s;

            auto styleInput = [&](UIInput& input)
            {
                input.paddingX = 6.0f * s;
                input.paddingY = 4.0f * s;
                input.background = field;
                input.hoverFill = Color(0.16f, 0.18f, 0.24f, 1.0f);
                input.border.color = accent;
                input.border.thickness = 1.0f;
                input.caret.color = ink;
                input.selection = selected;
                if (UIText* label = input.label())
                {
                    styleText(*label, mFont, fontPx, ink);
                }
                input.writeClipboard = mHost.writeClipboard;
                input.readClipboard = mHost.readClipboard;
            };
            auto styleCellInput = [&](UIInput& input, const Color& fill)
            {
                input.paddingX = textPadX;
                input.paddingY = 0.0f;
                input.background = fill;
                input.hoverFill = fill;
                input.border.color = clear;
                input.border.thickness = 0.0f;
                input.caret.color = ink;
                input.selection = selected;
                if (UIText* label = input.label())
                {
                    styleText(*label, mFont, fontPx, ink);
                }
                input.writeClipboard = mHost.writeClipboard;
                input.readClipboard = mHost.readClipboard;
            };
            auto centerText = [&](const std::shared_ptr<UIText>& text, float x, float width, float cellH)
            {
                text->shape.pivot = Vector2(0.0f, 0.0f);
                text->layout();
                float h = text->shape.size.y;
                if (h < 1.0f)
                {
                    h = 22.0f * s;
                }
                float w = text->shape.size.x;
                if (w < 1.0f || w > width)
                {
                    w = width;
                }
                setBox(text, Rect(x, (cellH - h) * 0.5f, w, h));
            };

            UIWidget* rootRaw = root.get();
            auto showPopup = [this, rootRaw, s, fontPx, popup, selected, ink, gate](UIWidget& anchor, const std::vector<std::pair<const char*, std::function<void()>>>& entries)
            {
                auto menu = std::make_shared<UIMenu>();
                menu->direction = UIDirection::Vertical;
                menu->padding = 4.0f * s;
                menu->spacing = 2.0f * s;
                menu->color = popup;
                menu->overlay = true;
                menu->shape.pivot = Vector2(0.0f, 0.0f);
                for (const auto& entry : entries)
                {
                    auto item = std::make_shared<UIMenuItem>();
                    item->paddingX = 12.0f * s;
                    item->paddingY = 8.0f * s;
                    item->background = popup;
                    item->hoverFill = selected;
                    item->setText(entry.first);
                    if (UIText* text = item->label())
                    {
                        styleText(*text, mFont, fontPx, ink);
                    }
                    auto action = entry.second;
                    UIMenu* menuRaw = menu.get();
                    item->onClick = [menuRaw, action]()
                    {
                        menuRaw->visible = false;
                        action();
                    };
                    menu->addItem(item);
                }
                rootRaw->attachChild(menu);
                if (gate->extra != nullptr) gate->extra->visible = false;
                gate->extra = menu.get();
                if (gate->file != nullptr) gate->file->visible = false;
                if (gate->dismiss != nullptr) gate->dismiss->visible = true;
                Rect bounds = anchor.screenBounds();
                Vector2 local = rootRaw->screenToLocal(Vector2(bounds.origin.x, bounds.origin.y + bounds.size.y));
                menu->shape.setBox(local, Vector2(180.0f * s, 36.0f * s * static_cast<float>(entries.size()) + 12.0f * s));
            };

            UITableRow* header = table->row(0);
            const bool allSelected = mSelect == SelectKind::All;
            Vector2 cornerSize = innerBox(0, 0, headW, rowH);
            auto corner = std::make_shared<UIWidget>();
            corner->shape.pivot = Vector2(0.0f, 0.0f);
            corner->color = allSelected ? selected : headerBg;
            corner->focusable = true;
            setBox(corner, Rect(0.0f, 0.0f, cornerSize.x, cornerSize.y));
            corner->onClick = [this]() { this->selectAll(); };
            if (allSelected && focus == nullptr)
            {
                focus = corner.get();
            }
            paintCell(header->cell(0), 0, 0, allSelected ? selected : headerBg);
            header->setContent(0, corner);
            for (int col = 0; col < cols; ++col)
            {
                TableColumn info = mDoc.column(static_cast<u32_t>(col));
                bool columnSelected = mSelect == SelectKind::Column && mSelectCol == col;
                Color headFill = (columnSelected || allSelected) ? selected : headerBg;
                Vector2 hitSize = innerBox(0, col + 1, colW, rowH);
                auto hit = std::make_shared<UIWidget>();
                hit->shape.pivot = Vector2(0.0f, 0.0f);
                hit->color = headFill;
                setBox(hit, Rect(0.0f, 0.0f, hitSize.x, hitSize.y));
                hit->onClick = [this, col]()
                {
                    auto now = std::chrono::steady_clock::now();
                    bool repeat = mClickHeader && mClickCol == col && now - mClickTime < std::chrono::milliseconds(450);
                    mClickTime = now;
                    mClickHeader = true;
                    mClickCol = col;
                    mClickRow = -1;
                    mSelect = SelectKind::Column;
                    mSelectCol = col;
                    if (repeat)
                    {
                        mEdit = EditKind::Header;
                        mEditCol = col;
                        mEditSeed = mDoc.column(static_cast<u32_t>(col)).name;
                        this->requestRebuild();
                        return;
                    }
                    this->requestRebuild();
                };
                if (mEdit == EditKind::Header && mEditCol == col)
                {
                    auto input = std::make_shared<EscapeInput>();
                    input->shape.pivot = Vector2(0.0f, 0.0f);
                    setBox(input, Rect(0.0f, 0.0f, hitSize.x, hitSize.y));
                    styleCellInput(*input, headFill);
                    input->setText(mEditSeed);
                    UIInput* inputRaw = input.get();
                    input->onSubmit = [this](const String& text) { this->finishEdit(text, 0, 0); };
                    input->onEscape = [this]() { this->handleEscape(); };
                    input->onFocusLoss = [this, inputRaw]()
                    {
                        if (mEdit == EditKind::Header)
                        {
                            this->finishEdit(inputRaw->text, 0, 0);
                        }
                    };
                    hit->attachChild(input);
                    focus = input.get();
                }
                else
                {
                    const float moreW = 28.0f * s;
                    const float moreH = 24.0f * s;
                    const float moreGap = 4.0f * s;
                    float nameW = hitSize.x - textPadX * 2.0f;
                    if (columnSelected)
                    {
                        nameW = hitSize.x - moreW - moreGap * 2.0f - textPadX;
                        if (nameW < 0.0f)
                        {
                            nameW = 0.0f;
                        }
                    }
                    auto nameText = makeText(info.name, mFont, fontPx, ink, nameW, 22.0f * s);
                    centerText(nameText, textPadX, nameW, hitSize.y);
                    hit->attachChild(nameText);
                    if (columnSelected)
                    {
                        auto more = makeMoreButton(mFont, fontPx, moreW, moreH, ink);
                        setBox(more, Rect(hitSize.x - moreW - moreGap, (hitSize.y - moreH) * 0.5f, moreW, moreH));
                        UIButton* moreRaw = more.get();
                        more->onClick = [this, col, moreRaw, showPopup]()
                        {
                            showPopup(*moreRaw, {
                                {"字段设置", [this, col]() { mSettingsCol = col; mDateRow = -1; mDateCol = -1; mColorIndex = -1; this->requestRebuild(); }},
                                {"在左边插入一列", [this, col]() { mDoc.insertColumn(static_cast<u32_t>(col)); mSelect = SelectKind::Column; mSelectCol = col; this->requestRebuild(); }},
                                {"在右边插入一列", [this, col]() { mDoc.insertColumn(static_cast<u32_t>(col + 1)); mSelect = SelectKind::Column; mSelectCol = col + 1; this->requestRebuild(); }},
                            });
                        };
                        hit->attachChild(more);
                    }
                }
                if (columnSelected && focus == nullptr)
                {
                    focus = hit.get();
                }
                paintCell(header->cell(col + 1), 0, col + 1, headFill);
                header->setContent(col + 1, hit);
            }

            for (int row = 0; row < rows; ++row)
            {
                bool rowSelected = mSelect == SelectKind::Row && mSelectRow == row;
                Color rowFill = (rowSelected || allSelected) ? selected : headerBg;
                Vector2 rowHitSize = innerBox(row + 1, 0, headW, rowH);
                auto rowHit = std::make_shared<UIWidget>();
                rowHit->shape.pivot = Vector2(0.0f, 0.0f);
                rowHit->color = rowFill;
                setBox(rowHit, Rect(0.0f, 0.0f, rowHitSize.x, rowHitSize.y));
                rowHit->onClick = [this, row]()
                {
                    mSelect = SelectKind::Row;
                    mSelectRow = row;
                    mPickRow = -1;
                    mPickCol = -1;
                    this->requestRebuild();
                };
                const float moreW = 28.0f * s;
                const float moreH = 24.0f * s;
                String rowLabel = String::format("%d", row + 1);
                float numW = 28.0f * s;
                float numH = 22.0f * s;
                if (UIFont* font = UIFont::find(mFont))
                {
                    if (font->isOpen())
                    {
                        Vector2 measured = font->measure(rowLabel.cstr(), rowLabel.length(), fontPx);
                        numW = (float)(int)(measured.x + 0.5f);
                        numH = (float)(int)(measured.y + 0.5f);
                    }
                }
                if (numW < 1.0f)
                {
                    numW = 1.0f;
                }
                if (numH < 1.0f)
                {
                    numH = 1.0f;
                }
                auto number = makeText(rowLabel, mFont, fontPx, ink, numW, numH);
                number->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(number, Rect(rowHitSize.x - numW - textPadX, (rowHitSize.y - numH) * 0.5f, numW, numH));
                rowHit->attachChild(number);
                if (rowSelected)
                {
                    auto more = makeMoreButton(mFont, fontPx, moreW, moreH, ink);
                    setBox(more, Rect(2.0f * s, (rowHitSize.y - moreH) * 0.5f, moreW, moreH));
                    UIButton* moreRaw = more.get();
                    more->onClick = [this, row, moreRaw, showPopup]()
                    {
                        showPopup(*moreRaw, {
                            {"删除当前行", [this, row]() { mDoc.deleteRow(static_cast<u32_t>(row)); mSelect = SelectKind::Cell; mSelectRow = clampi(row, 0, static_cast<int>(mDoc.rowCount()) - 1); mSelectCol = 0; this->requestRebuild(); }},
                            {"在上方插入一行", [this, row]() { mDoc.insertRow(static_cast<u32_t>(row)); mSelect = SelectKind::Row; mSelectRow = row; this->requestRebuild(); }},
                            {"在下方插入一行", [this, row]() { mDoc.insertRow(static_cast<u32_t>(row + 1)); mSelect = SelectKind::Row; mSelectRow = row + 1; this->requestRebuild(); }},
                        });
                    };
                    rowHit->attachChild(more);
                }
                paintCell(table->row(row + 1)->cell(0), row + 1, 0, rowFill);
                table->row(row + 1)->setContent(0, rowHit);

                for (int col = 0; col < cols; ++col)
                {
                    TableColumn info = mDoc.column(static_cast<u32_t>(col));
                    bool cellSelected = mSelect == SelectKind::Cell && mSelectRow == row && mSelectCol == col;
                    bool columnSelected = mSelect == SelectKind::Column && mSelectCol == col;
                    bool editing = mEdit == EditKind::Cell && mEditRow == row && mEditCol == col;
                    String shown = mDoc.cellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
                    Color fill = cellSelected ? selected : paper;
                    Color textColor = ink;
                    if (info.kind == FieldKind::Url && !shown.isEmpty())
                    {
                        textColor = url;
                    }
                    if (info.kind == FieldKind::SingleOption && !shown.isEmpty())
                    {
                        for (const FieldOption& option : info.options)
                        {
                            if (option.label == shown)
                            {
                                Color chip;
                                if (colorFromHex(option.color, chip))
                                {
                                    fill = chip;
                                    float lum = chip.r * 0.3f + chip.g * 0.59f + chip.b * 0.11f;
                                    textColor = lum > 0.62f ? Color(0.08f, 0.09f, 0.12f, 1.0f) : Color(0.96f, 0.97f, 0.98f, 1.0f);
                                }
                                break;
                            }
                        }
                    }
                    if (cellSelected || rowSelected || columnSelected || allSelected)
                    {
                        fill = selected;
                    }
                    Vector2 hitSize = innerBox(row + 1, col + 1, colW, rowH);
                    auto hit = std::make_shared<UIWidget>();
                    hit->shape.pivot = Vector2(0.0f, 0.0f);
                    hit->color = fill;
                    setBox(hit, Rect(0.0f, 0.0f, hitSize.x, hitSize.y));
                    hit->focusable = true;
                    hit->onClick = [this, row, col, info]()
                    {
                        if (mHost.ctrlDown && mHost.ctrlDown() && info.kind == FieldKind::Url && mHost.openExternal)
                        {
                            String url = mDoc.cellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
                            if (!url.isEmpty())
                            {
                                mHost.openExternal(url);
                                return;
                            }
                        }
                        this->selectCell(row, col);
                    };
                    hit->onKeyPress = [this, row, col](const UIKeyEvent& event)
                    {
                        const UIKeyCode key = event.key;
                        const bool ctrl = event.ctrl;
                        if (mConfirm != ConfirmKind::None)
                        {
                            return;
                        }
                        if (key == UIKeyCode::Escape)
                        {
                            this->handleEscape();
                            return;
                        }
                        if (this->handleShortcut(key, ctrl))
                        {
                            return;
                        }
                        if (ctrl && key == UIKeyCode::C)
                        {
                            String text = mDoc.cellText(static_cast<u32_t>(row), static_cast<u32_t>(col));
                            if (mHost.writeClipboard && !text.isEmpty())
                            {
                                mHost.writeClipboard(text);
                            }
                            return;
                        }
                        if (ctrl && key == UIKeyCode::V)
                        {
                            if (mHost.readClipboard)
                            {
                                mDoc.setCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), mHost.readClipboard());
                                this->requestRebuild();
                            }
                            return;
                        }
                        if (key == UIKeyCode::Left) { mSelectCol = clampi(col - 1, 0, static_cast<int>(mDoc.columnCount()) - 1); mSelectRow = row; mSelect = SelectKind::Cell; this->requestRebuild(); return; }
                        if (key == UIKeyCode::Right || key == UIKeyCode::Tab) { mSelectCol = clampi(col + 1, 0, static_cast<int>(mDoc.columnCount()) - 1); mSelectRow = row; mSelect = SelectKind::Cell; this->requestRebuild(); return; }
                        if (key == UIKeyCode::Up) { mSelectRow = clampi(row - 1, 0, static_cast<int>(mDoc.rowCount()) - 1); mSelectCol = col; mSelect = SelectKind::Cell; this->requestRebuild(); return; }
                        if (key == UIKeyCode::Down) { mSelectRow = clampi(row + 1, 0, static_cast<int>(mDoc.rowCount()) - 1); mSelectCol = col; mSelect = SelectKind::Cell; this->requestRebuild(); return; }
                        if (key == UIKeyCode::Enter || key == UIKeyCode::F2)
                        {
                            this->beginEdit(row, col, mDoc.cellText(static_cast<u32_t>(row), static_cast<u32_t>(col)));
                            return;
                        }
                        if (key == UIKeyCode::Delete || key == UIKeyCode::Backspace)
                        {
                            mDoc.setCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), String::empty);
                            this->requestRebuild();
                        }
                    };
                    hit->onKeyChar = [this, row, col](uint32_t character)
                    {
                        if (mConfirm != ConfirmKind::None || character < 32 || character == 127)
                        {
                            return;
                        }
                        this->beginEdit(row, col, UIFont::encodeUtf8(character));
                    };
                    if (editing)
                    {
                        auto input = std::make_shared<EscapeInput>();
                        input->shape.pivot = Vector2(0.0f, 0.0f);
                        setBox(input, Rect(0.0f, 0.0f, hitSize.x, hitSize.y));
                        styleCellInput(*input, fill);
                        input->setText(mEditSeed);
                        UIInput* inputRaw = input.get();
                        input->onSubmit = [this](const String& text) { this->finishEdit(text, 1, 0); };
                        input->onEscape = [this]() { this->handleEscape(); };
                        input->onFocusLoss = [this, inputRaw]()
                        {
                            if (mEdit == EditKind::Cell)
                            {
                                this->finishEdit(inputRaw->text, 0, 0);
                            }
                        };
                        input->onKeyPress = [this, inputRaw](const UIKeyEvent& event)
                        {
                            if (this->handleShortcut(event.key, event.ctrl))
                            {
                                return;
                            }
                            if (event.key == UIKeyCode::Tab)
                            {
                                this->finishEdit(inputRaw->text, 0, 1);
                            }
                        };
                        hit->attachChild(input);
                        focus = input.get();
                    }
                    else
                    {
                        auto label = makeText(shown, mFont, fontPx, textColor, hitSize.x - textPadX * 2.0f, 22.0f * s);
                        centerText(label, textPadX, hitSize.x - textPadX * 2.0f, hitSize.y);
                        hit->attachChild(label);
                        if (cellSelected && mPickRow == row && mPickCol == col && info.kind == FieldKind::SingleOption)
                        {
                            auto menu = std::make_shared<UIMenu>();
                            menu->direction = UIDirection::Vertical;
                            menu->padding = 4.0f * s;
                            menu->spacing = 2.0f * s;
                            menu->color = popup;
                            menu->overlay = true;
                            menu->shape.pivot = Vector2(0.0f, 0.0f);
                            if (info.options.empty())
                            {
                                auto item = std::make_shared<UIMenuItem>();
                                item->setText("无可选选项");
                                item->background = popup;
                                if (UIText* text = item->label()) styleText(*text, mFont, fontPx, mute);
                                menu->addItem(item);
                            }
                            for (const FieldOption& option : info.options)
                            {
                                auto item = std::make_shared<UIMenuItem>();
                                item->paddingX = 10.0f * s;
                                item->paddingY = 6.0f * s;
                                item->background = popup;
                                item->hoverFill = selected;
                                item->setText(option.label);
                                if (UIText* text = item->label()) styleText(*text, mFont, fontPx, ink);
                                String label = option.label;
                                item->onClick = [this, row, col, label]()
                                {
                                    mDoc.setCellText(static_cast<u32_t>(row), static_cast<u32_t>(col), label);
                                    mPickRow = -1;
                                    mPickCol = -1;
                                    this->requestRebuild();
                                };
                                menu->addItem(item);
                            }
                            menu->shape.setBox(Vector2(0.0f, rowH), Vector2(colW, 32.0f * s * static_cast<float>(info.options.empty() ? 1 : info.options.size()) + 8.0f * s));
                            hit->attachChild(menu);
                        }
                    }
                    if (focus == nullptr && cellSelected && !editing)
                    {
                        focus = hit.get();
                    }
                    paintCell(table->row(row + 1)->cell(col + 1), row + 1, col + 1, fill);
                    table->row(row + 1)->setContent(col + 1, hit);
                }
            }

            view->addChild(table);
            root->attachChild(view);
            view->setScroll(keepX, keepY);
            int revealRow = -1;
            int revealCol = -1;
            if (mEdit == EditKind::Cell)
            {
                revealRow = mEditRow;
                revealCol = mEditCol;
            }
            else if (mDateRow >= 0 && mDateCol >= 0)
            {
                revealRow = mDateRow;
                revealCol = mDateCol;
            }
            else if (mSelect == SelectKind::Cell)
            {
                revealRow = mSelectRow;
                revealCol = mSelectCol;
            }
            revealCell(*view, revealRow, revealCol, headW, colW, rowH);

            auto paintDrawer = [&](const std::shared_ptr<UIWidget>& drawerBody)
            {
                drawerBody->shape.pivot = Vector2(0.0f, 0.0f);
                drawerBody->color = Color(0.120f, 0.130f, 0.165f, 1.0f);
                setBox(drawerBody, Rect(gridW, chromeH, drawerW, gridH));
                root->attachChild(drawerBody);
            };

            if (mSettingsCol >= 0)
            {
                TableColumn info = mDoc.column(static_cast<u32_t>(mSettingsCol));
                auto drawerBody = std::make_shared<UIList>();
                drawerBody->direction = UIDirection::Vertical;
                drawerBody->padding = 16.0f * s;
                drawerBody->spacing = 10.0f * s;
                paintDrawer(drawerBody);
                float innerW = drawerW - 32.0f * s;
                auto head = std::make_shared<UIWidget>();
                head->pickable = false;
                head->shape.pivot = Vector2(0.0f, 0.0f);
                head->color = clear;
                setBox(head, Rect(0.0f, 0.0f, innerW, 32.0f * s));
                auto title = makeText("字段设置", mFont, fontPx + 1.0f, ink, innerW - 76.0f * s, 28.0f * s);
                title->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(title, Rect(0.0f, 2.0f * s, innerW - 76.0f * s, 28.0f * s));
                head->attachChild(title);
                auto close = makeButton("关闭", mFont, smallPx, 64.0f * s, 28.0f * s, Color(0.227f, 0.247f, 0.306f, 1.0f), ink);
                close->shape.setBox(Vector2(innerW - 64.0f * s, 2.0f * s), Vector2(64.0f * s, 28.0f * s));
                close->onClick = [this]() { mSettingsCol = -1; mColorIndex = -1; this->requestRebuild(); };
                head->attachChild(close);
                drawerBody->attachChild(head);
                drawerBody->attachChild(makeText("字段名称", mFont, smallPx, mute, 200.0f * s, 18.0f * s));
                auto name = std::make_shared<UIInput>();
                name->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(name, Rect(0.0f, 0.0f, drawerW - 40.0f * s, 32.0f * s));
                styleInput(*name);
                name->setText(info.name);
                int settingsCol = mSettingsCol;
                UIInput* nameRaw = name.get();
                name->onFocusLoss = [this, nameRaw, settingsCol]()
                {
                    mDoc.setColumnName(static_cast<u32_t>(settingsCol), nameRaw->text);
                    this->requestRebuild();
                };
                name->onSubmit = [this, settingsCol](const String& text)
                {
                    mDoc.setColumnName(static_cast<u32_t>(settingsCol), text);
                    this->requestRebuild();
                };
                drawerBody->attachChild(name);
                drawerBody->attachChild(makeText("字段类型", mFont, smallPx, mute, 200.0f * s, 18.0f * s));
                auto drop = std::make_shared<UIDropdown>();
                drop->overlay = true;
                drop->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(drop, Rect(0.0f, 0.0f, drawerW - 40.0f * s, 32.0f * s));
                drop->background = field;
                drop->text.color = ink;
                drop->placeholderStyle.color = mute;
                if (UIText* caption = drop->caption())
                {
                    styleText(*caption, mFont, fontPx, ink);
                }
                for (int i = 0; i < 8; ++i)
                {
                    drop->addItem(i, fieldKindLabel(kKinds[i]));
                }
                int currentKind = 0;
                for (int i = 0; i < 8; ++i)
                {
                    if (kKinds[i] == info.kind) currentKind = i;
                }
                drop->setValue(currentKind);
                drop->onValueChanged = [this, settingsCol](int index)
                {
                    if (index < 0 || index > 7) return;
                    FieldKind kind = kKinds[index];
                    if (kind == mDoc.column(static_cast<u32_t>(settingsCol)).kind) return;
                    if (mDoc.columnChangeClears(static_cast<u32_t>(settingsCol), kind))
                    {
                        mPendingKind = kind;
                        mConfirm = ConfirmKind::ChangeType;
                        mSettingsCol = settingsCol;
                    }
                    else
                    {
                        mDoc.setColumnType(static_cast<u32_t>(settingsCol), kind);
                    }
                    this->requestRebuild();
                };
                drawerBody->attachChild(drop);

                if (info.kind == FieldKind::SingleOption)
                {
                    drawerBody->attachChild(makeText("选项", mFont, smallPx, mute, 200.0f * s, 18.0f * s));
                    for (int i = 0; i < static_cast<int>(info.options.size()); ++i)
                    {
                        auto line = std::make_shared<UIList>();
                        line->direction = UIDirection::Horizontal;
                        line->padding = 0.0f;
                        line->spacing = 6.0f * s;
                        line->color = clear;
                        line->shape.pivot = Vector2(0.0f, 0.0f);
                        auto label = std::make_shared<UIInput>();
                        label->shape.pivot = Vector2(0.0f, 0.0f);
                        setBox(label, Rect(0.0f, 0.0f, 150.0f * s, 30.0f * s));
                        styleInput(*label);
                        label->setText(info.options[static_cast<size_t>(i)].label);
                        UIInput* labelRaw = label.get();
                        label->onFocusLoss = [this, settingsCol, i, labelRaw]()
                        {
                            auto options = mDoc.column(static_cast<u32_t>(settingsCol)).options;
                            if (i < 0 || i >= static_cast<int>(options.size())) return;
                            options[static_cast<size_t>(i)].label = labelRaw->text;
                            mDoc.setOptions(static_cast<u32_t>(settingsCol), options);
                            this->requestRebuild();
                        };
                        line->attachChild(label);
                        Color chip(0.39f, 0.40f, 0.95f, 1.0f);
                        colorFromHex(info.options[static_cast<size_t>(i)].color, chip);
                        auto swatch = makeButton(" ", mFont, smallPx, 28.0f * s, 28.0f * s, chip, ink);
                        swatch->onClick = [this, i]()
                        {
                            mColorIndex = mColorIndex == i ? -1 : i;
                            this->requestRebuild();
                        };
                        line->attachChild(swatch);
                        auto up = makeButton("上", mFont, smallPx, 32.0f * s, 28.0f * s, Color(0.227f, 0.247f, 0.306f, 1.0f), ink);
                        up->onClick = [this, settingsCol, i]()
                        {
                            if (i <= 0) return;
                            auto options = mDoc.column(static_cast<u32_t>(settingsCol)).options;
                            std::swap(options[static_cast<size_t>(i)], options[static_cast<size_t>(i - 1)]);
                            mDoc.setOptions(static_cast<u32_t>(settingsCol), options);
                            mColorIndex = -1;
                            this->requestRebuild();
                        };
                        auto down = makeButton("下", mFont, smallPx, 32.0f * s, 28.0f * s, Color(0.227f, 0.247f, 0.306f, 1.0f), ink);
                        down->onClick = [this, settingsCol, i]()
                        {
                            auto options = mDoc.column(static_cast<u32_t>(settingsCol)).options;
                            if (i + 1 >= static_cast<int>(options.size())) return;
                            std::swap(options[static_cast<size_t>(i)], options[static_cast<size_t>(i + 1)]);
                            mDoc.setOptions(static_cast<u32_t>(settingsCol), options);
                            mColorIndex = -1;
                            this->requestRebuild();
                        };
                        auto remove = makeButton("删", mFont, smallPx, 32.0f * s, 28.0f * s, Color(0.45f, 0.22f, 0.24f, 1.0f), ink);
                        remove->onClick = [this, settingsCol, i]()
                        {
                            auto options = mDoc.column(static_cast<u32_t>(settingsCol)).options;
                            if (i < 0 || i >= static_cast<int>(options.size())) return;
                            options.erase(options.begin() + i);
                            mDoc.setOptions(static_cast<u32_t>(settingsCol), options);
                            mColorIndex = -1;
                            this->requestRebuild();
                        };
                        line->attachChild(up);
                        line->attachChild(down);
                        line->attachChild(remove);
                        drawerBody->attachChild(line);
                        if (mColorIndex == i)
                        {
                            auto colors = std::make_shared<UIWidget>();
                            colors->shape.pivot = Vector2(0.0f, 0.0f);
                            colors->color = clear;
                            setBox(colors, Rect(0.0f, 0.0f, drawerW - 40.0f * s, 120.0f * s));
                            int presetCount = static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0]));
                            for (int p = 0; p < presetCount; ++p)
                            {
                                Color chipColor;
                                colorFromHex(kPresets[p], chipColor);
                                auto chipButton = makeButton(" ", mFont, smallPx, 22.0f * s, 22.0f * s, chipColor, ink);
                                float x = static_cast<float>(p % 10) * 26.0f * s;
                                float y = static_cast<float>(p / 10) * 26.0f * s;
                                chipButton->shape.pivot = Vector2(0.0f, 0.0f);
                                chipButton->shape.setBox(Vector2(x, y), Vector2(22.0f * s, 22.0f * s));
                                String hex = kPresets[p];
                                chipButton->onClick = [this, settingsCol, i, hex]()
                                {
                                    auto options = mDoc.column(static_cast<u32_t>(settingsCol)).options;
                                    if (i < 0 || i >= static_cast<int>(options.size())) return;
                                    options[static_cast<size_t>(i)].color = hex;
                                    mDoc.setOptions(static_cast<u32_t>(settingsCol), options);
                                    this->requestRebuild();
                                };
                                colors->attachChild(chipButton);
                            }
                            drawerBody->attachChild(colors);
                            auto hex = std::make_shared<UIInput>();
                            hex->shape.pivot = Vector2(0.0f, 0.0f);
                            setBox(hex, Rect(0.0f, 0.0f, drawerW - 40.0f * s, 30.0f * s));
                            styleInput(*hex);
                            hex->placeholder = "#rrggbb";
                            hex->setText(info.options[static_cast<size_t>(i)].color);
                            hex->onSubmit = [this, settingsCol, i](const String& text)
                            {
                                Color parsed;
                                if (!colorFromHex(text, parsed))
                                {
                                    mDoc.setStatus("颜色需要是 #rrggbb");
                                    this->requestRebuild();
                                    return;
                                }
                                auto options = mDoc.column(static_cast<u32_t>(settingsCol)).options;
                                if (i < 0 || i >= static_cast<int>(options.size())) return;
                                String value = trimCopy(text);
                                if (value.isEmpty() || value.at(0) != '#') value = String("#") + value;
                                options[static_cast<size_t>(i)].color = value;
                                mDoc.setOptions(static_cast<u32_t>(settingsCol), options);
                                this->requestRebuild();
                            };
                            drawerBody->attachChild(hex);
                        }
                    }
                    auto add = makeButton("添加选项", mFont, smallPx, 120.0f * s, 30.0f * s, accent, ink);
                    add->onClick = [this, settingsCol]()
                    {
                        auto options = mDoc.column(static_cast<u32_t>(settingsCol)).options;
                        FieldOption option;
                        option.label = String::format("选项%d", static_cast<int>(options.size()) + 1);
                        option.color = "#6366f1";
                        options.push_back(option);
                        mDoc.setOptions(static_cast<u32_t>(settingsCol), options);
                        this->requestRebuild();
                    };
                    drawerBody->attachChild(add);
                }

                auto removeField = makeButton("删除字段", mFont, fontPx, 120.0f * s, 32.0f * s, Color(0.55f, 0.24f, 0.27f, 1.0f), ink);
                removeField->onClick = [this, settingsCol]()
                {
                    mConfirm = ConfirmKind::DeleteColumn;
                    mSettingsCol = settingsCol;
                    this->requestRebuild();
                };
                drawerBody->attachChild(removeField);
            }
            else if (mDateRow >= 0 && mDateCol >= 0)
            {
                auto drawerBody = std::make_shared<UIList>();
                drawerBody->direction = UIDirection::Vertical;
                drawerBody->padding = 16.0f * s;
                drawerBody->spacing = 10.0f * s;
                paintDrawer(drawerBody);
                float innerW = drawerW - 32.0f * s;
                auto head = std::make_shared<UIWidget>();
                head->pickable = false;
                head->shape.pivot = Vector2(0.0f, 0.0f);
                head->color = clear;
                setBox(head, Rect(0.0f, 0.0f, innerW, 32.0f * s));
                auto title = makeText("日期时间选择", mFont, fontPx + 1.0f, ink, innerW - 76.0f * s, 28.0f * s);
                title->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(title, Rect(0.0f, 2.0f * s, innerW - 76.0f * s, 28.0f * s));
                head->attachChild(title);
                auto close = makeButton("关闭", mFont, smallPx, 64.0f * s, 28.0f * s, Color(0.227f, 0.247f, 0.306f, 1.0f), ink);
                close->shape.setBox(Vector2(innerW - 64.0f * s, 2.0f * s), Vector2(64.0f * s, 28.0f * s));
                close->onClick = [this]() { mDateRow = -1; mDateCol = -1; this->requestRebuild(); };
                head->attachChild(close);
                drawerBody->attachChild(head);
                drawerBody->attachChild(makeText("日期", mFont, smallPx, mute, 200.0f * s, 18.0f * s));
                auto date = std::make_shared<EscapeInput>();
                date->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(date, Rect(0.0f, 0.0f, drawerW - 40.0f * s, 32.0f * s));
                styleInput(*date);
                date->placeholder = "YYYY-MM-DD";
                date->setText(mDateDraft);
                date->onValueChanged = [this](const String& text) { mDateDraft = text; };
                date->onEscape = [this]() { this->handleEscape(); };
                drawerBody->attachChild(date);
                drawerBody->attachChild(makeText("时间", mFont, smallPx, mute, 200.0f * s, 18.0f * s));
                auto time = std::make_shared<EscapeInput>();
                time->shape.pivot = Vector2(0.0f, 0.0f);
                setBox(time, Rect(0.0f, 0.0f, drawerW - 40.0f * s, 32.0f * s));
                styleInput(*time);
                time->placeholder = "HH:MM:SS";
                time->setText(mTimeDraft);
                time->onValueChanged = [this](const String& text) { mTimeDraft = text; };
                time->onEscape = [this]() { this->handleEscape(); };
                drawerBody->attachChild(time);
                auto actions = std::make_shared<UIList>();
                actions->direction = UIDirection::Horizontal;
                actions->padding = 0.0f;
                actions->spacing = 8.0f * s;
                actions->color = clear;
                actions->shape.pivot = Vector2(0.0f, 0.0f);
                auto clearBtn = makeButton("清除", mFont, fontPx, 80.0f * s, 32.0f * s, Color(0.227f, 0.247f, 0.306f, 1.0f), ink);
                int dateRow = mDateRow;
                int dateCol = mDateCol;
                clearBtn->onClick = [this, dateRow, dateCol]()
                {
                    mDoc.setCellText(static_cast<u32_t>(dateRow), static_cast<u32_t>(dateCol), String::empty);
                    mDateRow = -1;
                    mDateCol = -1;
                    this->requestRebuild();
                };
                auto ok = makeButton("确定", mFont, fontPx, 80.0f * s, 32.0f * s, accent, ink);
                ok->onClick = [this, dateRow, dateCol]()
                {
                    String combined = trimCopy(mDateDraft);
                    String time = trimCopy(mTimeDraft);
                    if (!time.isEmpty())
                    {
                        combined = combined + "T" + time;
                    }
                    if (!validateField(FieldKind::DateTime, combined))
                    {
                        mDoc.setStatus("日期时间格式无效");
                        this->requestRebuild();
                        return;
                    }
                    mDoc.setCellText(static_cast<u32_t>(dateRow), static_cast<u32_t>(dateCol), combined);
                    mDateRow = -1;
                    mDateCol = -1;
                    this->requestRebuild();
                };
                actions->attachChild(clearBtn);
                actions->attachChild(ok);
                drawerBody->attachChild(actions);
            }
        }

        if (mConfirm != ConfirmKind::None)
        {
            auto shade = std::make_shared<UIWidget>();
            shade->overlay = true;
            shade->shape.pivot = Vector2(0.0f, 0.0f);
            shade->color = Color(0.0f, 0.0f, 0.0f, 0.45f);
            setBox(shade, Rect(0.0f, 0.0f, mWidth, mHeight));
            auto panel = std::make_shared<UIList>();
            panel->direction = UIDirection::Vertical;
            panel->padding = 16.0f * s;
            panel->spacing = 12.0f * s;
            panel->color = Color(0.145f, 0.157f, 0.196f, 1.0f);
            panel->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(panel, Rect((mWidth - 420.0f * s) * 0.5f, (mHeight - 160.0f * s) * 0.5f, 420.0f * s, 150.0f * s));
            const char* message = mConfirm == ConfirmKind::DeleteColumn
                ? "删除该字段？只剩一列时会清空内容并保留这一列。"
                : "部分单元格无法转换为新类型，这些单元格会被清空。是否继续？";
            panel->attachChild(makeText(message, mFont, fontPx, ink, 380.0f * s, 48.0f * s));
            auto actions = std::make_shared<UIList>();
            actions->direction = UIDirection::Horizontal;
            actions->padding = 0.0f;
            actions->spacing = 8.0f * s;
            actions->color = clear;
            actions->shape.pivot = Vector2(0.0f, 0.0f);
            auto cancel = makeButton("取消", mFont, fontPx, 80.0f * s, 32.0f * s, Color(0.227f, 0.247f, 0.306f, 1.0f), ink);
            cancel->onClick = [this]() { mConfirm = ConfirmKind::None; this->requestRebuild(); };
            auto ok = makeButton("确定", mFont, fontPx, 80.0f * s, 32.0f * s, accent, ink);
            auto escapeConfirm = [this](const UIKeyEvent& event)
            {
                if (event.key == UIKeyCode::Escape) this->handleEscape();
            };
            cancel->onKeyPress = escapeConfirm;
            ok->onKeyPress = escapeConfirm;
            ok->onClick = [this]()
            {
                if (mConfirm == ConfirmKind::ChangeType && mSettingsCol >= 0)
                {
                    mDoc.setColumnType(static_cast<u32_t>(mSettingsCol), mPendingKind);
                }
                else if (mConfirm == ConfirmKind::DeleteColumn && mSettingsCol >= 0)
                {
                    int col = mSettingsCol;
                    mDoc.deleteColumn(static_cast<u32_t>(col));
                    mSettingsCol = -1;
                    mColorIndex = -1;
                    if (mDoc.isOpen())
                    {
                        mSelectCol = clampi(col, 0, static_cast<int>(mDoc.columnCount()) - 1);
                    }
                }
                mConfirm = ConfirmKind::None;
                this->requestRebuild();
            };
            actions->attachChild(cancel);
            actions->attachChild(ok);
            panel->attachChild(actions);
            shade->attachChild(panel);
            root->attachChild(shade);
            focus = ok.get();
        }

        mWindow->setRoot(root);
        if (focus != nullptr)
        {
            mWindow->setFocus(focus);
        }
        else
        {
            mWindow->setFocus(root.get());
        }
        this->notifyTitle();
    }

}
