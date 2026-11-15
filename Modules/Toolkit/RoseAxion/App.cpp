#include "./App.h"
#include "./AboutDialog.h"
#include "./Prefs.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"
#include "UI/UIWindow.h"

#include <cmath>
#include <utility>

namespace eokas::RoseAxion {

    namespace {

        struct MenuGate
        {
            UIMenu* file = nullptr;
            UIMenu* help = nullptr;
            UIWidget* dismiss = nullptr;
            void hide()
            {
                if (file != nullptr) file->visible = false;
                if (help != nullptr) help->visible = false;
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
        loadPrefs();
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
        return fileTitleFromPath(mDoc.path());
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
        host.pickFilePath = mHost.pickFilePath;
        host.pickFolderPath = mHost.pickFolderPath;
        host.ctrlDown = mHost.ctrlDown;
        host.handleShortcut = [this](UIKeyCode key, bool ctrl) { return this->handleShortcut(key, ctrl); };
        host.confirmChangeType = [this](int col, const String& typeName)
        {
            mConfirmCol = col;
            mPendingType = typeName;
            mConfirm = ConfirmKind::ChangeType;
            this->requestRebuild();
        };
        host.confirmDeleteColumn = [this](int col)
        {
            mConfirmCol = col;
            mConfirm = ConfirmKind::DeleteColumn;
            this->requestRebuild();
        };
        host.confirmDeleteTable = [this](int index)
        {
            mConfirmTable = index;
            mConfirm = ConfirmKind::DeleteTable;
            this->requestRebuild();
        };
        host.confirmDeleteSchema = [this](int index)
        {
            mConfirmSchema = index;
            mConfirm = ConfirmKind::DeleteSchema;
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
        mConfirmTable = -1;
        mConfirmSchema = -1;
        mBoundTable = -1;
        if (mEditor)
        {
            mEditor->resetOutliner();
            mEditor->reset();
            mEditor->resetSchema();
            mEditor->selectTables();
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
            mDoc.setStatus(DocStatus::CannotOpenFile);
            this->requestRebuild();
            return;
        }
        if (mWindow != nullptr)
        {
            mWindow->clearPressed();
        }
        String path = mHost.pickOpenPath();
        if (mWindow != nullptr)
        {
            mWindow->clearPressed();
        }
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
            mDoc.setStatus(DocStatus::CannotSaveFile);
            this->requestRebuild();
            return false;
        }
        if (mWindow != nullptr)
        {
            mWindow->clearPressed();
        }
        String suggested = mDoc.path().isEmpty() ? String("table.rose") : mDoc.path();
        String path = mHost.pickSavePath(suggested);
        if (mWindow != nullptr)
        {
            mWindow->clearPressed();
        }
        if (path.isEmpty())
        {
            return false;
        }
        bool ok = mDoc.save(path);
        this->requestRebuild();
        return ok;
    }

    void AxionApp::showAbout()
    {
        mAboutOpen = true;
        this->requestRebuild();
    }

    bool AxionApp::handleEscape()
    {
        if (mAboutOpen)
        {
            mAboutOpen = false;
            this->requestRebuild();
            return true;
        }
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

    bool AxionApp::colorDropping() const
    {
        return mEditor && mEditor->colorDropping();
    }

    void AxionApp::sampleColorDropper(float x, float y, bool commit)
    {
        if (mEditor)
        {
            mEditor->sampleColorDropper(x, y, commit);
        }
    }

    void AxionApp::cancelColorDropper()
    {
        if (mEditor)
        {
            mEditor->cancelColorDropper();
        }
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
        const float titlePx = 20.0f * s;
        const float menuPx = menuFontPx(s);
        const float statusH = 26.0f * s;
        const RoseTheme& t = theme();
        const Color bar = t.bar;
        const Color ink = t.ink;
        const Color mute = t.mute;
        const Color accent = t.accent;
        const Color popup = t.popup;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const Color warn = t.warn;
        const Color quiet = t.quiet;
        const bool showStatus = mDoc.isOpen() || !mDoc.status().isEmpty();
        float headerH = 0.0f;
        float workY = 0.0f;
        float workH = 0.0f;

        auto root = std::make_shared<UIWidget>();
        root->shape.pivot = Vector2(0.0f, 0.0f);
        root->pickable = false;
        setBox(root, Rect(0.0f, 0.0f, mWidth, mHeight));
        fillSurface(*root, t.voidBg);
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

        auto header = std::make_shared<UIWidget>();
        header->shape.pivot = Vector2(0.0f, 0.0f);
        header->pickable = false;
        fillSurface(*header, bar);

        auto makePopupMenu = [&]()
        {
            auto menu = std::make_shared<UIMenu>();
            menu->direction = UIDirection::Vertical;
            menu->padding = 2.0f * s;
            menu->spacing = 1.0f * s;
            menu->color = popup;
            menu->overlay = true;
            menu->visible = false;
            menu->shape.pivot = Vector2(0.0f, 0.0f);
            return menu;
        };
        auto fileMenu = makePopupMenu();
        auto helpMenu = makePopupMenu();

        auto dismiss = std::make_shared<UIWidget>();
        dismiss->overlay = true;
        dismiss->visible = false;
        dismiss->pickable = true;
        dismiss->color = clear;
        dismiss->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(dismiss, Rect(0.0f, 0.0f, mWidth, mHeight));
        auto gate = std::make_shared<MenuGate>();
        gate->file = fileMenu.get();
        gate->help = helpMenu.get();
        gate->dismiss = dismiss.get();
        dismiss->onClick = [gate]() { gate->hide(); };

        auto addMenuItem = [&](UIMenu& menu, const char* label, const std::function<void()>& action)
        {
            auto item = std::make_shared<UIMenuItem>();
            item->paddingX = 10.0f * s;
            item->paddingY = 3.0f * s;
            item->setText(label);
            styleMenuItem(*item, mFont, menuPx);
            item->onClick = [action, gate]()
            {
                gate->hide();
                action();
            };
            menu.addItem(item);
            return item;
        };
        auto makeBarItem = [&](const char* label)
        {
            auto item = std::make_shared<UIMenuItem>();
            item->paddingX = 10.0f * s;
            item->paddingY = 2.0f * s;
            item->setText(label);
            styleMenuBarItem(*item, mFont, menuPx, bar);
            item->syncSize();
            return item;
        };
        auto fileItem = makeBarItem("文件");
        auto helpItem = makeBarItem("帮助");
        fileMenu->host = fileItem.get();
        helpMenu->host = helpItem.get();
        UIMenuItem* fileRaw = fileItem.get();
        UIMenuItem* helpRaw = helpItem.get();
        UIMenu* fileMenuRaw = fileMenu.get();
        UIMenu* helpMenuRaw = helpMenu.get();
        UIWidget* rootRawForMenu = root.get();
        auto openBarMenu = [rootRawForMenu, gate](UIMenuItem* host, UIMenu* menu, float minWidth)
        {
            bool open = !menu->visible;
            gate->hide();
            if (!open)
            {
                return;
            }
            menu->visible = true;
            if (gate->dismiss != nullptr)
            {
                gate->dismiss->visible = true;
            }
            menu->layout();
            Vector2 size = menu->shape.size;
            if (size.x < minWidth)
            {
                size.x = minWidth;
            }
            Rect bounds = host->screenBounds();
            Vector2 local = rootRawForMenu->screenToLocal(Vector2(bounds.origin.x, bounds.origin.y + bounds.size.y));
            menu->shape.pivot = Vector2(0.0f, 0.0f);
            menu->shape.setBox(local, size);
        };
        fileItem->onClick = [fileRaw, fileMenuRaw, openBarMenu, s]()
        {
            openBarMenu(fileRaw, fileMenuRaw, 160.0f * s);
        };
        helpItem->onClick = [helpRaw, helpMenuRaw, openBarMenu, s]()
        {
            openBarMenu(helpRaw, helpMenuRaw, 200.0f * s);
        };
        addMenuItem(*fileMenu, "新建", [this]() { this->newDocument(); });
        addMenuItem(*fileMenu, "打开...", [this]() { this->openDocument(); });
        addMenuItem(*fileMenu, "保存", [this]() { this->saveDocument(); });
        addMenuItem(*fileMenu, "另存为...", [this]() { this->saveDocumentAs(); });
        addMenuItem(*helpMenu, "关于 RoseAxion", [this]() { this->showAbout(); });

        auto left = std::make_shared<UIList>();
        left->direction = UIDirection::Horizontal;
        left->padding = 2.0f * s;
        left->spacing = 4.0f * s;
        left->centerCross = true;
        left->color = clear;
        left->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(left, Rect(0.0f, 0.0f, 0.0f, 0.0f));
        left->attachChild(fileItem);
        left->attachChild(helpItem);
        left->layout();
        headerH = left->shape.size.y;
        setBox(header, Rect(0.0f, 0.0f, mWidth, headerH));
        setBox(left, Rect(0.0f, 0.0f, left->shape.size.x, headerH));
        left->layout();
        header->attachChild(left);
        workY = headerH;
        workH = mHeight - headerH - (showStatus ? statusH : 0.0f);
        if (workH < 40.0f)
        {
            workH = 40.0f;
        }

        if (mDoc.isOpen())
        {
            String name = this->fileTitle();
            if (mDoc.dirty())
            {
                name = name + " *";
            }
            float nameMax = mWidth - left->shape.size.x - 24.0f * s;
            if (nameMax < 48.0f * s)
            {
                nameMax = 48.0f * s;
            }
            auto nameText = makeText(name, mFont, smallPx, mute, nameMax, 20.0f * s);
            nameText->layout();
            float nw = nameText->shape.size.x;
            float nh = nameText->shape.size.y;
            float nx = mWidth - nw - 16.0f * s;
            if (nx < left->shape.size.x + 8.0f * s)
            {
                nx = left->shape.size.x + 8.0f * s;
            }
            setBox(nameText, Rect(nx, (headerH - nh) * 0.5f, nw, nh));
            header->attachChild(nameText);
        }

        auto headerRule = makeSplit(mWidth, false);
        setBox(headerRule, Rect(0.0f, headerH - 1.0f, mWidth, 1.0f));
        header->attachChild(headerRule);
        root->attachChild(header);

        if (!mDoc.isOpen())
        {
            auto splash = std::make_shared<PaintPanel>();
            splash->fill(t.voidBg);
            splash->hair = t.hair;
            if (t.glow)
            {
                splash->orbs = {
                    { mWidth * 0.78f, -40.0f * s, 220.0f * s, t.orbA, 0.2f, true },
                    { mWidth * 0.92f, mHeight * 0.42f, 180.0f * s, t.orbB, 1.7f, true },
                    { -30.0f * s, mHeight * 0.72f, 160.0f * s, t.orbC, 2.9f, true },
                };
            }
            setBox(splash, Rect(0.0f, workY, mWidth, workH));
            root->attachChild(splash);
            float textMax = mWidth - 80.0f * s;
            if (textMax < 200.0f * s)
            {
                textMax = mWidth - 32.0f * s;
            }
            auto title = makeText("RoseAxion", mFont, titlePx, ink, textMax, 32.0f * s);
            title->layout();
            auto hint = makeText("打开 Library 或新建表格开始", mFont, fontPx, mute, textMax, 28.0f * s);
            hint->layout();
            auto actions = std::make_shared<UIList>();
            actions->direction = UIDirection::Horizontal;
            actions->padding = 0.0f;
            actions->spacing = 12.0f * s;
            actions->centerCross = true;
            actions->color = clear;
            actions->shape.pivot = Vector2(0.0f, 0.0f);
            auto create = makeButton("新建表格", mFont, fontPx, 128.0f * s, 36.0f * s, accent, t.inkOnAccent);
            create->onClick = [this]() { this->newDocument(); };
            auto open = makeButton("打开 Library", mFont, fontPx, 128.0f * s, 36.0f * s, quiet, ink);
            open->onClick = [this]() { this->openDocument(); };
            actions->attachChild(create);
            actions->attachChild(open);
            setBox(actions, Rect(0.0f, 0.0f, 0.0f, 0.0f));
            actions->layout();
            float clusterH = title->shape.size.y + 10.0f * s + hint->shape.size.y + 24.0f * s + actions->shape.size.y;
            float clusterY = workY + (workH - clusterH) * 0.5f;
            if (clusterY < workY + 24.0f * s)
            {
                clusterY = workY + 24.0f * s;
            }
            setBox(title, Rect((mWidth - title->shape.size.x) * 0.5f, clusterY, title->shape.size.x, title->shape.size.y));
            setBox(hint, Rect((mWidth - hint->shape.size.x) * 0.5f, clusterY + title->shape.size.y + 10.0f * s, hint->shape.size.x, hint->shape.size.y));
            setBox(actions, Rect((mWidth - actions->shape.size.x) * 0.5f, clusterY + title->shape.size.y + 10.0f * s + hint->shape.size.y + 24.0f * s, actions->shape.size.x, actions->shape.size.y));
            root->attachChild(title);
            root->attachChild(hint);
            root->attachChild(actions);
            focus = root.get();
        }
        else
        {
            int activeTable = mDoc.hasActiveTable() ? static_cast<int>(mDoc.activeTableIndex()) : -1;
            if (activeTable != mBoundTable)
            {
                if (mEditor)
                {
                    mEditor->reset();
                }
                keepX = 0.0f;
                keepY = 0.0f;
                mBoundTable = activeTable;
            }
            if (!mEditor)
            {
                mEditor = std::make_shared<RoseEditor>();
            }
            mEditor->bind(mDoc, this->editorHost());
            mEditor->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(mEditor, Rect(0.0f, workY, mWidth, workH));
            mEditor->setScroll(keepX, keepY);
            mEditor->rebuild();
            root->attachChild(mEditor);
            focus = mEditor->focusWidget();
            if (focus == nullptr)
            {
                focus = mEditor.get();
            }
        }

        if (showStatus)
        {
            auto statusBar = std::make_shared<UIWidget>();
            statusBar->shape.pivot = Vector2(0.0f, 0.0f);
            statusBar->pickable = false;
            fillSurface(*statusBar, bar);
            setBox(statusBar, Rect(0.0f, mHeight - statusH, mWidth, statusH));
            auto statusRule = makeSplit(mWidth, false);
            setBox(statusRule, Rect(0.0f, 0.0f, mWidth, 1.0f));
            statusBar->attachChild(statusRule);
            String statusText = mDoc.status();
            Color statusColor = docStatusFailed(mDoc.statusKind()) ? warn : mute;
            if (statusText.isEmpty())
            {
                statusText = String("就绪");
            }
            float statusW = mWidth - 24.0f * s;
            if (statusW < 40.0f * s)
            {
                statusW = 40.0f * s;
            }
            auto status = makeText(statusText, mFont, smallPx, statusColor, statusW, 18.0f * s);
            status->layout();
            setBox(status, Rect(12.0f * s, (statusH - status->shape.size.y) * 0.5f, status->shape.size.x, status->shape.size.y));
            statusBar->attachChild(status);
            root->attachChild(statusBar);
        }

        root->attachChild(dismiss);
        root->attachChild(fileMenu);
        root->attachChild(helpMenu);

        if (mAboutOpen)
        {
            auto about = std::make_shared<RoseAboutDialog>();
            RoseAboutDialog::Host aboutHost;
            aboutHost.font = mFont;
            aboutHost.scale = mScale;
            aboutHost.openExternal = mHost.openExternal;
            aboutHost.onClose = [this]()
            {
                mAboutOpen = false;
                this->requestRebuild();
            };
            about->bind(aboutHost);
            about->rebuild(mWidth, mHeight);
            root->attachChild(about);
            if (UIWidget* aboutFocus = about->focusWidget())
            {
                focus = aboutFocus;
            }
        }

        if (mConfirm != ConfirmKind::None)
        {
            const bool unsaved = mConfirm == ConfirmKind::Unsaved;
            auto shade = std::make_shared<UIWidget>();
            shade->overlay = true;
            shade->shape.pivot = Vector2(0.0f, 0.0f);
            shade->color = t.shade;
            setBox(shade, Rect(0.0f, 0.0f, mWidth, mHeight));
            const char* message = "部分单元格无法转换为新类型，这些单元格会被清空。是否继续？";
            if (unsaved)
            {
                message = "当前表格尚未保存，是否保存？";
            }
            else if (mConfirm == ConfirmKind::DeleteColumn)
            {
                message = "删除该字段？只剩一列时会清空内容并保留这一列。";
            }
            else if (mConfirm == ConfirmKind::DeleteTable)
            {
                message = "删除该表格？此操作无法撤销。";
            }
            else if (mConfirm == ConfirmKind::DeleteSchema)
            {
                message = "删除该模式？此操作无法撤销。";
            }
            const float padX = 24.0f * s;
            const float padY = 20.0f * s;
            const float gap = 18.0f * s;
            float textMax = mWidth - padX * 2.0f - 48.0f * s;
            if (textMax < 200.0f * s)
            {
                textMax = mWidth - 32.0f * s;
            }
            auto text = makeText(message, mFont, fontPx, ink, textMax, 0.0f);
            text->layout();
            auto actions = std::make_shared<UIList>();
            actions->direction = UIDirection::Horizontal;
            actions->padding = 0.0f;
            actions->spacing = 8.0f * s;
            actions->centerCross = true;
            actions->color = clear;
            actions->shape.pivot = Vector2(0.0f, 0.0f);
            auto cancel = makeButton("取消", mFont, fontPx, 80.0f * s, 32.0f * s, quiet, ink);
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
                auto discard = makeDangerButton("不保存", mFont, fontPx, 96.0f * s, 32.0f * s);
                auto save = makeButton("保存", mFont, fontPx, 80.0f * s, 32.0f * s, accent, t.inkOnAccent);
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
                auto ok = makeDangerButton("确定", mFont, fontPx, 80.0f * s, 32.0f * s);
                ok->onKeyPress = escapeConfirm;
                ok->onClick = [this]()
                {
                    if (mConfirm == ConfirmKind::ChangeType && mConfirmCol >= 0)
                    {
                        mDoc.setColumnType(static_cast<u32_t>(mConfirmCol), mPendingType);
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
                    else if (mConfirm == ConfirmKind::DeleteTable && mConfirmTable >= 0)
                    {
                        mDoc.deleteTable(static_cast<u32_t>(mConfirmTable));
                    }
                    else if (mConfirm == ConfirmKind::DeleteSchema && mConfirmSchema >= 0)
                    {
                        mDoc.deleteUserSchema(static_cast<u32_t>(mConfirmSchema));
                    }
                    mConfirm = ConfirmKind::None;
                    mConfirmCol = -1;
                    mConfirmTable = -1;
                    mConfirmSchema = -1;
                    this->requestRebuild();
                };
                actions->attachChild(cancel);
                actions->attachChild(ok);
                focus = ok.get();
            }
            setBox(actions, Rect(0.0f, 0.0f, 0.0f, 0.0f));
            actions->layout();
            float innerW = text->shape.size.x;
            if (actions->shape.size.x > innerW)
            {
                innerW = actions->shape.size.x;
            }
            float panelW = innerW + padX * 2.0f;
            float panelH = padY + text->shape.size.y + gap + actions->shape.size.y + padY;
            float panelX = (mWidth - panelW) * 0.5f;
            float panelY = (mHeight - panelH) * 0.5f;
            if (panelX < 16.0f * s)
            {
                panelX = 16.0f * s;
            }
            if (panelY < 16.0f * s)
            {
                panelY = 16.0f * s;
            }
            auto panel = std::make_shared<UIWidget>();
            panel->pickable = true;
            panel->color = t.raised;
            panel->shape.pivot = Vector2(0.0f, 0.0f);
            setBox(panel, Rect(panelX, panelY, panelW, panelH));
            setBox(text, Rect(padX, padY, text->shape.size.x, text->shape.size.y));
            setBox(actions, Rect(panelW - padX - actions->shape.size.x, padY + text->shape.size.y + gap, actions->shape.size.x, actions->shape.size.y));
            panel->attachChild(text);
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
