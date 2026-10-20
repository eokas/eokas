#include "./MainWindow.h"

namespace eokas {

    namespace {

        const Color kCanvas(0.102f, 0.110f, 0.133f, 1.0f);

    }

    class MainWindow::Frame : public UIWidget {
    public:
        std::shared_ptr<MainMenu> menu;
        std::shared_ptr<UIWidget> content;

        Frame() {
            shape.pivot = Vector2(0.0f, 0.0f);
            pickable = false;
            color = kCanvas;
        }

        void layout() override {
            if (!visible) {
                return;
            }

            float barHeight = 0.0f;
            if (menu) {
                menu->resize(Vector2(shape.size.x, menu->shape.size.y));
                menu->layout();
                barHeight = menu->shape.size.y;
                this->placeChild(*menu, Vector2(0.0f, 0.0f), Vector2(shape.size.x, barHeight));
            }

            if (content) {
                float contentHeight = shape.size.y - barHeight;
                if (contentHeight < 0.0f) {
                    contentHeight = 0.0f;
                }
                this->placeChild(*content, Vector2(0.0f, barHeight), Vector2(shape.size.x, contentHeight));
                content->layout();
            }
        }

        void render(UIPrimitive& primitive) override {
            if (!visible) {
                return;
            }
            primitive.addQuad(matrixLocalToScreen(), Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), color);
            UIWidget::render(primitive);
        }
    };

    MainWindow::MainWindow() {
        mTitle = String("eokas");

        mFrame = std::make_shared<Frame>();
        mFrame->menu = std::make_shared<MainMenu>();
        mFrame->attachChild(mFrame->menu);

        mFrame->content = std::make_shared<UIWidget>();
        mFrame->content->shape.pivot = Vector2(0.0f, 0.0f);
        mFrame->content->pickable = false;
        mFrame->attachChild(mFrame->content);

        mFont = std::make_unique<UIFont>();
        if (File::exists(MainMenu::kFontPath) && mFont->open(MainMenu::kFontPath, (uint32_t)MainMenu::kFontSize)) {
            UIFont::bind(MainMenu::kFontPath, mFont.get());
        }

        this->bindNativeInput();
    }

#if !defined(_WIN32)

    void MainWindow::bindNativeInput() {
    }

#endif

    MainWindow::~MainWindow() {
        this->closeNative();
        if (mUiReady) {
            mUIWindow.quit();
            mUiReady = false;
        }
        mRenderer = nullptr;
    }

    void MainWindow::setTitle(const char* title) {
        if (title == nullptr || title[0] == '\0') {
            mTitle = String("eokas");
        } else {
            mTitle = String(title);
        }
        if (mNativeWindow.isOpen()) {
            mNativeWindow.setTitle(mTitle);
        }
    }

    void MainWindow::setSize(float width, float height) {
        if (width < 0.0f) {
            width = 0.0f;
        }
        if (height < 0.0f) {
            height = 0.0f;
        }

        if (!this->isOpen()) {
            this->ensureOpen(width, height);
            return;
        }
        mNativeWindow.setClientSize(width, height);
        this->applyClientSize(width, height);
    }

    void MainWindow::setRenderer(Renderer& renderer) {
        if (mRenderer != &renderer) {
            this->releaseSurface();
            mRenderer = &renderer;
        }
        this->ensureSurface();
    }

    void MainWindow::tick(f32_t deltaTime) {
        mNativeWindow.pumpMessages();
        if (!this->isOpen() || !mUiReady) {
            return;
        }
        mUIWindow.tick(deltaTime);
        this->publishFont();
        if (mRenderer != nullptr && mSurface && mSpace.activeCamera) {
            mRenderer->render(mSurface, mSpace);
        }
    }

    void MainWindow::ensureSurface() {
        void* windowHandle = this->handle();
        if (mRenderer == nullptr || windowHandle == nullptr || !mUiReady) {
            return;
        }

        uint32_t width = (uint32_t)mFrame->shape.size.x;
        uint32_t height = (uint32_t)mFrame->shape.size.y;
        if (width == 0) {
            width = 1;
        }
        if (height == 0) {
            height = 1;
        }

        if (!mSurface) {
            mSurface = mRenderer->attach(windowHandle, width, height);
        } else if (mSurface->getWindowHandle() != windowHandle) {
            mSurface.reset();
            mSurface = mRenderer->attach(windowHandle, width, height);
        } else if (mSurface->getWidth() != width || mSurface->getHeight() != height) {
            mRenderer->resize(mSurface, width, height);
        }
        if (!mSurface) {
            return;
        }

        if (!mCamera) {
            mCamera = std::make_shared<Camera>();
            mCamera->viewport.front = 0.0f;
            mCamera->viewport.back = 1.0f;
            mSpace.clearColor = kCanvas;
            mSpace.add(mCamera);
            mSpace.activeCamera = mCamera;
        }
        mCamera->viewport.left = 0.0f;
        mCamera->viewport.top = 0.0f;
        mCamera->viewport.right = (float)width;
        mCamera->viewport.bottom = (float)height;

        UIPrimitive::Ref primitive = mUIWindow.primitive();
        if (!primitive) {
            return;
        }
        for (const Primitive::Ref& item : mSpace.primitives) {
            if (item == primitive) {
                return;
            }
        }
        mSpace.add(primitive);
    }

    void MainWindow::releaseSurface() {
        if (mCamera) {
            mSpace.remove(mCamera);
            mCamera.reset();
        }
        if (UIPrimitive::Ref primitive = mUIWindow.primitive()) {
            mSpace.remove(primitive);
        }
        mSurface.reset();
        mAtlasReady = false;
    }

    void MainWindow::publishFont() {
        if (!mFont || !mFont->isOpen()) {
            return;
        }
        UIPrimitive::Ref primitive = mUIWindow.primitive();
        if (!primitive) {
            return;
        }
        bool dirty = mFont->takeAtlasDirty();
        if (!mAtlasReady || dirty) {
            primitive->setPendingUpload(mFont->atlasRgba(), mFont->atlasSize());
            mAtlasReady = true;
        }
    }

    void MainWindow::applyClientSize(float width, float height) {
        if (width < 0.0f) {
            width = 0.0f;
        }
        if (height < 0.0f) {
            height = 0.0f;
        }

        mFrame->shape.pivot = Vector2(0.0f, 0.0f);
        mFrame->shape.setBox(Vector2(0.0f, 0.0f), Vector2(width, height));
        if (!mUiReady) {
            mUIWindow.init((uint32_t)width, (uint32_t)height);
            mUIWindow.setRoot(mFrame);
            mUiReady = true;
        }
        mFrame->layout();
        this->ensureSurface();
    }

    MainMenu& MainWindow::menu() {
        return *mFrame->menu;
    }

    const MainMenu& MainWindow::menu() const {
        return *mFrame->menu;
    }

    UIWidget& MainWindow::content() {
        return *mFrame->content;
    }

    const UIWidget& MainWindow::content() const {
        return *mFrame->content;
    }

    const Surface::Ref& MainWindow::surface() const {
        return mSurface;
    }

    UIWindow& MainWindow::uiWindow() {
        return mUIWindow;
    }

    const UIWindow& MainWindow::uiWindow() const {
        return mUIWindow;
    }

    void MainWindow::ensureOpen(float width, float height) {
        if (mNativeWindow.isOpen()) {
            return;
        }
        if (!mNativeWindow.create(mTitle, width, height)) {
            return;
        }
        mNativeWindow.show();
        Vector2 size = mNativeWindow.clientSize();
        if (size.x < 1.0f) {
            size.x = width;
        }
        if (size.y < 1.0f) {
            size.y = height;
        }
        this->applyClientSize(size.x, size.y);
    }

    void MainWindow::closeNative() {
        mNativeWindow.destroy();
    }

    bool MainWindow::isOpen() const {
        return mNativeWindow.isOpen();
    }

    void* MainWindow::handle() const {
        return mNativeWindow.handle();
    }

}
