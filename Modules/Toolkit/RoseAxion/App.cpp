#include "./App.h"

#include "UI/Main.h"
#include "UI/UIFont.h"
#include "UI/UIWindow.h"

#include <utility>

namespace eokas::RoseAxion {

    namespace {

        void setBox(UIWidget& widget, const Rect& box)
        {
            widget.shape.size = box.size;
            widget.shape.origin = box.origin + widget.shape.pivot * widget.shape.size;
        }

        void setBox(const std::shared_ptr<UIWidget>& widget, const Rect& box)
        {
            setBox(*widget, box);
        }

        void styleText(UIText& text, const String& font, float size, const Color& color)
        {
            text.style.fontPath = font;
            text.style.fontSize = size;
            text.style.color = color;
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

    RoseEditor::Host AxionApp::editorHost()
    {
        RoseEditor::Host host;
        host.requestRebuild = [this]() { this->requestRebuild(); };
        host.writeClipboard = mHost.writeClipboard;
        host.readClipboard = mHost.readClipboard;
        host.openExternal = mHost.openExternal;
        host.ctrlDown = mHost.ctrlDown;
        host.handleShortcut = [this](UIKeyCode key, bool ctrl) { return this->handleShortcut(key, ctrl); };
        host.confirmChangeType = [this](int col, FieldKind kind)
        {
            mConfirmCol = col;
            mPendingKind = kind;
            mConfirm = ConfirmKind::ChangeType;
            this->requestRebuild();
        };
        host.confirmDeleteColumn = [this](int col)
        {
            mConfirmCol = col;
            mConfirm = ConfirmKind::DeleteColumn;
            this->requestRebuild();
        };
        host.font = mFont;
        host.scale = mScale;
        return host;
    }

    void AxionApp::resetView()
    {
        mConfirm = ConfirmKind::None;
        mPending = PendingAction::None;
        mResetScroll = true;
        mConfirmCol = -1;
        if (mEditor)
        {
            mEditor->reset();
        }
    }

    bool AxionApp::confirmUnsaved(PendingAction action)
    {
        if (!mDoc.dirty())
        {
            return true;
        }
        mPending = action;
        mConfirm = ConfirmKind::Unsaved;
        this->requestRebuild();
        return false;
    }

    void AxionApp::runPending()
    {
        PendingAction action = mPending;
        mPending = PendingAction::None;
        mConfirm = ConfirmKind::None;
        if (action == PendingAction::New)
        {
            this->applyNewDocument();
            return;
        }
        if (action == PendingAction::Open)
        {
            this->applyOpenDocument();
            return;
        }
        if (action == PendingAction::Close)
        {
            mCloseAllowed = true;
            if (mHost.requestQuit)
            {
                mHost.requestQuit();
            }
            return;
        }
        this->requestRebuild();
    }

    void AxionApp::applyNewDocument()
    {
        mDoc.createNew();
        this->resetView();
        this->requestRebuild();
    }

    void AxionApp::applyOpenDocument()
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
            this->requestRebuild();
            return;
        }
        if (mDoc.load(path))
        {
            this->resetView();
        }
        this->requestRebuild();
    }

    void AxionApp::newDocument()
    {
        if (!this->confirmUnsaved(PendingAction::New))
        {
            return;
        }
        this->applyNewDocument();
    }

    void AxionApp::openDocument()
    {
        if (!this->confirmUnsaved(PendingAction::Open))
        {
            return;
        }
        this->applyOpenDocument();
    }

    bool AxionApp::requestClose()
    {
        if (mCloseAllowed || !mDoc.dirty())
        {
            return true;
        }
        mPending = PendingAction::Close;
        mConfirm = ConfirmKind::Unsaved;
        this->requestRebuild();
        return false;
    }

    bool AxionApp::saveDocument()
    {
        if (!mDoc.isOpen())
        {
            return false;
        }
        if (!mDoc.path().isEmpty())
        {
            bool ok = mDoc.save(mDoc.path());
            this->requestRebuild();
            return ok;
        }
        return this->saveDocumentAs();
    }

    bool AxionApp::saveDocumentAs()
    {
        if (!mDoc.isOpen())
        {
            return false;
        }
        if (!mHost.pickSavePath)
        {
            mDoc.setStatus("无法保存文件");
            this->requestRebuild();
            return false;
        }
        String suggested = mDoc.path().isEmpty() ? String("table.rose") : mDoc.path();
        String path = mHost.pickSavePath(suggested);
        if (path.isEmpty())
        {
            return false;
        }
        bool ok = mDoc.save(path);
        this->requestRebuild();
        return ok;
    }

    bool AxionApp::handleEscape()
    {
        if (mConfirm != ConfirmKind::None)
        {
            mConfirm = ConfirmKind::None;
            mPending = PendingAction::None;
            this->requestRebuild();
            return true;
        }
        if (mEditor && mEditor->handleEscape())
        {
            return true;
        }
        return false;
    }

    bool AxionApp::resizeCursorAt(float x, float y) const
    {
        return mEditor && mEditor->resizeCursorAt(x, y);
    }

    void AxionApp::build()
    {
        if (mWindow == nullptr)
        {
            return;
        }
        float keepX = 0.0f;
        float keepY = 0.0f;
        if (!mResetScroll && mEditor)
        {
            keepX = mEditor->scrollX();
            keepY = mEditor->scrollY();
        }
        mResetScroll = false;
        // Drop window focus/hover before RoseEditor::rebuild() destroys
        // the previously focused cell. setRoot(nullptr) keeps mEditor alive
        // via the shared_ptr, but clears dangling UIWindow pointers.
        mWindow->setRoot(nullptr);

        UIWidget* focus = nullptr;
        const float s = mScale;
        const float fontPx = 15.0f * s;
        const float smallPx = 13.0f * s;
        const float barH = 52.0f * s;
        const Color canvas(0.102f, 0.110f, 0.133f, 1.0f);
        const Color bar(0.078f, 0.086f, 0.110f, 1.0f);
        const Color selected(0.275f, 0.400f, 0.620f, 1.0f);
        const Color ink(0.925f, 0.933f, 0.953f, 1.0f);
        const Color mute(0.620f, 0.660f, 0.720f, 1.0f);
        const Color accent(0.345f, 0.545f, 0.930f, 1.0f);
        const Color popup(0.160f, 0.176f, 0.220f, 1.0f);
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const Color warn(0.96f, 0.72f, 0.42f, 1.0f);

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
                fileMenuRaw->shape.setBox(local, Vector2(160.0f * s, 36.0f * s * 4.0f + 12.0f * s));
            }
        };
        addMenuItem(*fileMenu, "新建", [this]() { this->newDocument(); });
        addMenuItem(*fileMenu, "打开...", [this]() { this->openDocument(); });
        addMenuItem(*fileMenu, "保存", [this]() { this->saveDocument(); });
        addMenuItem(*fileMenu, "另存为...", [this]() { this->saveDocumentAs(); });

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
            if (!mEditor)
            {
                mEditor = std::make_shared<RoseEditor>();
            }
            mEditor->bind(mDoc, this->editorHost());
            mEditor->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(mEditor, Rect(0.0f, chromeH, mWidth, gridH));
            mEditor->setScroll(keepX, keepY);
            mEditor->rebuild();
            root->attachChild(mEditor);
            focus = mEditor->focusWidget();
            if (focus == nullptr)
            {
                focus = mEditor.get();
            }
        }

        if (mConfirm != ConfirmKind::None)
        {
            const bool unsaved = mConfirm == ConfirmKind::Unsaved;
            float panelW = unsaved ? 480.0f * s : 420.0f * s;
            float panelH = unsaved ? 168.0f * s : 150.0f * s;
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
            setBox(panel, Rect((mWidth - panelW) * 0.5f, (mHeight - panelH) * 0.5f, panelW, panelH));
            const char* message = "部分单元格无法转换为新类型，这些单元格会被清空。是否继续？";
            if (unsaved)
            {
                message = "当前表格尚未保存，是否保存？";
            }
            else if (mConfirm == ConfirmKind::DeleteColumn)
            {
                message = "删除该字段？只剩一列时会清空内容并保留这一列。";
            }
            panel->attachChild(makeText(message, mFont, fontPx, ink, panelW - 40.0f * s, 48.0f * s));
            auto actions = std::make_shared<UIList>();
            actions->direction = UIDirection::Horizontal;
            actions->padding = 0.0f;
            actions->spacing = 8.0f * s;
            actions->color = clear;
            actions->shape.pivot = Vector2(0.0f, 0.0f);
            auto cancel = makeButton("取消", mFont, fontPx, 80.0f * s, 32.0f * s, Color(0.227f, 0.247f, 0.306f, 1.0f), ink);
            cancel->onClick = [this]()
            {
                mConfirm = ConfirmKind::None;
                mPending = PendingAction::None;
                this->requestRebuild();
            };
            auto escapeConfirm = [this](const UIKeyEvent& event)
            {
                if (event.key == UIKeyCode::Escape) this->handleEscape();
            };
            cancel->onKeyPress = escapeConfirm;
            if (unsaved)
            {
                auto discard = makeButton("不保存", mFont, fontPx, 96.0f * s, 32.0f * s, Color(0.227f, 0.247f, 0.306f, 1.0f), ink);
                auto save = makeButton("保存", mFont, fontPx, 80.0f * s, 32.0f * s, accent, ink);
                discard->onKeyPress = escapeConfirm;
                save->onKeyPress = escapeConfirm;
                discard->onClick = [this]() { this->runPending(); };
                save->onClick = [this]()
                {
                    if (!this->saveDocument())
                    {
                        return;
                    }
                    this->runPending();
                };
                actions->attachChild(cancel);
                actions->attachChild(discard);
                actions->attachChild(save);
                focus = save.get();
            }
            else
            {
                auto ok = makeButton("确定", mFont, fontPx, 80.0f * s, 32.0f * s, accent, ink);
                ok->onKeyPress = escapeConfirm;
                ok->onClick = [this]()
                {
                    if (mConfirm == ConfirmKind::ChangeType && mConfirmCol >= 0)
                    {
                        mDoc.setColumnType(static_cast<u32_t>(mConfirmCol), mPendingKind);
                    }
                    else if (mConfirm == ConfirmKind::DeleteColumn && mConfirmCol >= 0)
                    {
                        int col = mConfirmCol;
                        mDoc.deleteColumn(static_cast<u32_t>(col));
                        if (mEditor)
                        {
                            mEditor->closeSettings();
                        }
                    }
                    mConfirm = ConfirmKind::None;
                    mConfirmCol = -1;
                    this->requestRebuild();
                };
                actions->attachChild(cancel);
                actions->attachChild(ok);
                focus = ok.get();
            }
            panel->attachChild(actions);
            shade->attachChild(panel);
            root->attachChild(shade);
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
