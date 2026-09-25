#ifndef _EOKAS_UI_DOCKING_H_
#define _EOKAS_UI_DOCKING_H_

#include "UIWidget.h"
#include <functional>
#include <memory>
#include <vector>

namespace eokas
{
    enum class UIDockMode
    {
        None,
        Fill,
        Left,
        Right,
        Top,
        Bottom
    };

    class UIDockSpace;

    class UIDockPage : public UIWidget
    {
    public:
        Color tabMark { 0.32f, 0.44f, 0.68f, 1.0f };
        Color activeColor { 0.32f, 0.40f, 0.56f, 1.0f };
        std::function<bool(UIDockPage&, float, float, int)> onDrag;
        std::function<void(UIDockPage&)> onDragEnd;

        UIDockPage();
        ~UIDockPage() override;

        UIDockMode mode() const { return mMode; }
        UIDockSpace* space() const { return mSpace; }
        void setDock(UIDockSpace* space, UIDockMode mode);
        const std::shared_ptr<UIWidget>& head() const { return mHead; }
        void setHead(const std::shared_ptr<UIWidget>& widget);
        const std::shared_ptr<UIWidget>& body() const { return mBody; }
        void setBody(const std::shared_ptr<UIWidget>& widget);
        void layout(const Rect& rect) override;
        void render(UIShape& shape) override;
        void layoutInWindow(float width, float height);
        void place(const Rect& headRect, const Rect& bodyRect, bool showBody, bool tabActive);

    private:
        UIDockMode mMode = UIDockMode::None;
        std::shared_ptr<UIWidget> mHead;
        std::shared_ptr<UIWidget> mBody;
        UIDockSpace* mSpace = nullptr;
        bool mDragging = false;
        bool mSuppressClick = false;
        bool mTabActive = false;

        void bindHead();
        void unbindHead();
        void syncChildren();
    };

    class UIDockSpace : public UIWidget
    {
    public:
        float splitterSize = 4.0f;
        float dropBand = 28.0f;
        float minPane = 48.0f;
        float dragSlop = 4.0f;
        Color emptyColor { 0.12f, 0.12f, 0.14f, 1.0f };
        Color borderColor { 0.55f, 0.58f, 0.66f, 1.0f };
        Color splitterColor { 0.55f, 0.58f, 0.66f, 1.0f };
        Color splitterHoverColor { 0.32f, 0.44f, 0.68f, 1.0f };
        Color previewColor { 0.32f, 0.44f, 0.68f, 0.35f };
        std::function<void(UIDockSpace&)> onDestroy;

        UIDockSpace();
        ~UIDockSpace() override;

        void setScreenMapper(const std::function<Rect(const Rect&)>& mapper);
        Rect toScreen(const Rect& local) const;
        void layout(const Rect& rect) override;
        bool pageBounds(UIDockPage* page, Rect& bounds);

        void addPage(const std::shared_ptr<UIDockPage>& page);
        void removePage(const std::shared_ptr<UIDockPage>& page);
        void dockPage(const std::shared_ptr<UIDockPage>& page, UIDockMode mode);
        std::shared_ptr<UIDockPage> takePage(UIDockPage* page);

        bool showPreview(float localX, float localY);
        void clearPreview();
        UIDockMode previewMode() const { return mPreviewMode; }
        void acceptDrop(const std::shared_ptr<UIDockPage>& page);
        void activate(UIDockPage* page);
        bool splitterAxis(float x, float y, bool& vertical) const;
        void setHover(float x, float y);

        void render(UIShape& shape) override;
        void triggerPointerDrag(float x, float y, int button) override;
        void triggerPointerRelease() override;

    private:
        struct Node;

        std::unique_ptr<Node> mRoot;
        std::function<Rect(const Rect&)> mScreenMapper;
        Node* mDragSplitter = nullptr;
        Node* mHoverSplitter = nullptr;
        bool mSplitterTracking = false;
        float mSplitterGrab = 0.0f;
        bool mPreview = false;
        UIDockMode mPreviewMode = UIDockMode::None;
        Node* mPreviewLeaf = nullptr;
        Rect mPreviewRect;

        void ensure(const std::shared_ptr<UIDockPage>& page);
        std::shared_ptr<UIDockPage> findShared(UIDockPage* page) const;
        bool extract(std::unique_ptr<Node>& node, UIDockPage* page);
        void reconcile(Node* node, UIDockMode side);
        void fillLeaf(Node* leaf, const std::shared_ptr<UIDockPage>& page);
        void splitLeaf(Node* leaf, const std::shared_ptr<UIDockPage>& page, UIDockMode mode);
        void splitRoot(const std::shared_ptr<UIDockPage>& page, UIDockMode mode);
        Node* findPageNode(Node* node, UIDockPage* page);
        Node* findLeaf(Node* node, float x, float y);
        Node* nearestLeaf(Node* node, float x, float y);
        Node* firstLeaf(Node* node);
        float tabStrip(const Node& node) const;
        float bandOf(const Rect& area) const;
        float fitRatio(float ratio, float inner) const;
        UIDockMode zoneAt(const Rect& area, float x, float y) const;
        Rect previewRectFor(const Rect& area, UIDockMode mode) const;
        Rect splitterRect(Node* node) const;
        Node* findSplitter(Node* node, float x, float y, float slop) const;
        void layoutTree();
        void layoutNode(Node* node, const Rect& area);
        void layoutLeaf(Node* node);
        void renderNode(Node* node, UIShape& shape, const Vector2& origin);
    };
}

#endif//_EOKAS_UI_DOCKING_H_
