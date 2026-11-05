#include "./Stage.h"

namespace eokas {

    namespace {

        const Color kCanvas(0.102f, 0.110f, 0.133f, 1.0f);
        const char* kFontPath = "Content/Fonts/RobotoRegular.ttf";
        const float kFontSize = 15.0f;

        UIKeyCode transferKeyCode(NativeKeyCode keyCode) {
            return static_cast<UIKeyCode>(static_cast<int>(keyCode));
        }

        UIPointerCode transferMouseButton(NativeMouseButton mouseButton) {
            return static_cast<UIPointerCode>(static_cast<int>(mouseButton));
        }

    }

    class Stage::Frame : public UIWidget {
    public:
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

            if (content) {
                this->placeChild(*content, Vector2(0.0f, 0.0f), shape.size);
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

    Stage::Stage() {
        mTitle = String("eokas");

        mFrame = std::make_shared<Frame>();
        mFrame->content = std::make_shared<UIWidget>();
        mFrame->content->shape.pivot = Vector2(0.0f, 0.0f);
        mFrame->content->pickable = false;
        mFrame->attachChild(mFrame->content);

        mFont = std::make_unique<UIFont>();
        if (File::exists(kFontPath) && mFont->open(kFontPath, (uint32_t)kFontSize)) {
            UIFont::bind(kFontPath, mFont.get());
        }

        this->bindNativeInput();
    }

    void Stage::bindNativeInput() {
        mNativeWindow.onClose = [this]() {
            this->releaseSurface();
        };
        mNativeWindow.onClientSize = [this](float width, float height) {
            if (!mNativeWindow.isOpen()) {
                return;
            }
            this->applyClientSize(width, height);
        };
        mNativeWindow.onMouseMove = [this](float x, float y) {
            if (mUiReady) {
                mUIWindow.onMouseMove(x, y);
            }
        };
        mNativeWindow.onMouseDown = [this](float x, float y, NativeMouseButton button) {
            if (mUiReady) {
                mUIWindow.onMouseDown(x, y, transferMouseButton(button));
            }
        };
        mNativeWindow.onMouseUp = [this](float x, float y, NativeMouseButton button) {
            if (mUiReady) {
                mUIWindow.onMouseUp(x, y, transferMouseButton(button));
            }
        };
        mNativeWindow.onMouseWheel = [this](float x, float y, float deltaX, float deltaY) {
            if (mUiReady) {
                mUIWindow.onMouseWheel(x, y, deltaX, deltaY);
            }
        };
        mNativeWindow.onKeyPress = [this](NativeKeyCode keyCode, bool, bool ctrl, bool shift, bool alt, bool repeat) {
            if (!mUiReady) {
                return false;
            }
            UIKeyEvent event;
            event.key = transferKeyCode(keyCode);
            event.ctrl = ctrl;
            event.shift = shift;
            event.alt = alt;
            event.repeat = repeat;
            mUIWindow.onKeyPress(event);
            return true;
        };
        mNativeWindow.onKeyChar = [this](uint32_t character) {
            if (!mUiReady) {
                return false;
            }
            mUIWindow.onKeyChar(character);
            return true;
        };
        mNativeWindow.onKeyRelease = [this](NativeKeyCode keyCode, bool, bool ctrl, bool shift, bool alt) {
            if (!mUiReady) {
                return false;
            }
            UIKeyEvent event;
            event.key = transferKeyCode(keyCode);
            event.ctrl = ctrl;
            event.shift = shift;
            event.alt = alt;
            mUIWindow.onKeyRelease(event);
            return true;
        };
    }

    Stage::~Stage() {
        this->closeNative();
        if (mUiReady) {
            mUIWindow.quit();
            mUiReady = false;
        }
        mRenderer = nullptr;
    }

    void Stage::setTitle(const char* title) {
        if (title == nullptr || title[0] == '\0') {
            mTitle = String("eokas");
        } else {
            mTitle = String(title);
        }
        if (mNativeWindow.isOpen()) {
            mNativeWindow.setTitle(mTitle);
        }
    }

    void Stage::setSize(float width, float height) {
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

    void Stage::setRenderer(Renderer& renderer) {
        if (mRenderer != &renderer) {
            this->releaseSurface();
            mRenderer = &renderer;
        }
        this->ensureSurface();
    }

    void Stage::tick(f32_t deltaTime) {
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

    void Stage::ensureSurface() {
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

    void Stage::releaseSurface() {
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

    void Stage::publishFont() {
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

    void Stage::applyClientSize(float width, float height) {
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

    UIWidget& Stage::content() {
        return *mFrame->content;
    }

    const UIWidget& Stage::content() const {
        return *mFrame->content;
    }

    const Surface::Ref& Stage::surface() const {
        return mSurface;
    }

    UIWindow& Stage::uiWindow() {
        return mUIWindow;
    }

    const UIWindow& Stage::uiWindow() const {
        return mUIWindow;
    }

    void Stage::ensureOpen(float width, float height) {
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

    void Stage::closeNative() {
        mNativeWindow.destroy();
    }

    bool Stage::isOpen() const {
        return mNativeWindow.isOpen();
    }

    void* Stage::handle() const {
        return mNativeWindow.handle();
    }

}
