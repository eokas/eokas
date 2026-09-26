#include "UIDocking.h"
#include "../UIFont.h"
#include "../UIStroke.h"
#include "UIList.h"
#include "UIText.h"
#include <cmath>

namespace eokas
{
    namespace
    {
        float clampf(float value, float lo, float hi)
        {
            if (value < lo) return lo;
            if (value > hi) return hi;
            return value;
        }
        float minf(float a, float b) { return a < b ? a : b; }
        float maxf(float a, float b) { return a > b ? a : b; }

        float snap(float value)
        {
            return floorf(value + 0.5f);
        }

        void textMetrics(UIText* text, float& scale, float& ascender, float& descender)
        {
            scale = 1.0f;
            ascender = 0.0f;
            descender = 0.0f;
            UIFont* font = text ? UIFont::find(text->style.fontPath) : nullptr;
            if (!text || font == nullptr || !font->isOpen()) return;
            font->drawMetrics(text->style.fontSize, scale, ascender, descender);
        }

        float textAdvance(UIText* text)
        {
            if (!text) return 0.0f;
            UIFont* font = UIFont::find(text->style.fontPath);
            if (font == nullptr || !font->isOpen()) return text->rect.size.x;
            float scale = 1.0f;
            float ascender = 0.0f;
            float descender = 0.0f;
            textMetrics(text, scale, ascender, descender);
            float width = 0.0f;
            size_t index = 0;
            while (index < text->text.length())
            {
                uint32_t codepoint = 0;
                if (!UIFont::nextUtf8(text->text.cstr(), text->text.length(), index, codepoint)) continue;
                width += font->glyphSized(codepoint, text->style.fontSize).advance * scale;
            }
            return width;
        }

        float textLine(UIText* text)
        {
            float scale = 1.0f;
            float ascender = 0.0f;
            float descender = 0.0f;
            textMetrics(text, scale, ascender, descender);
            float line = ascender - descender;
            if (line < 1.0f && text) line = text->rect.size.y;
            return line;
        }

        void hairline(UIPrimitive& primitive, float x, float y, float length, bool vertical, const Color& color)
        {
            float sx = snap(x);
            float sy = snap(y);
            float span = snap(length);
            if (span < 1.0f) return;
            Rect solid = UIFont::solidUV();
            if (vertical) primitive.addQuad(Rect(sx, sy, 1.0f, span), solid, color);
            else primitive.addQuad(Rect(sx, sy, span, 1.0f), solid, color);
        }

        void placeHeadContent(UIWidget* head)
        {
            if (!head) return;
            float padX = 8.0f;
            float spacing = 0.0f;
            if (UIList* layout = dynamic_cast<UIList*>(head))
            {
                padX = layout->padding;
                spacing = layout->spacing;
            }
            padX = snap(padX);
            Vector2 pen(snap(padX), 0.0f);
            float right = snap(head->rect.size.x) - padX;
            bool first = true;
            for (auto& child : head->children)
            {
                if (!child || !child->visible) continue;
                if (!first) pen.x += spacing;
                first = false;
                float w = child->rect.size.x;
                float h = child->rect.size.y;
                if (UIText* text = dynamic_cast<UIText*>(child.get()))
                {
                    w = ceilf(textAdvance(text));
                    h = textLine(text);
                    if (w < 1.0f) w = 1.0f;
                }
                if (right > pen.x && pen.x + w > right) w = right - pen.x;
                pen.y = (head->rect.size.y - h) * 0.5f;
                if (pen.y < 0.0f) pen.y = 0.0f;
                child->layout(Rect(Vector2(snap(pen.x), snap(pen.y)), Vector2(w, h)));
                pen.x += w;
            }
        }

        float headContentWidth(UIWidget* head)
        {
            if (!head) return 48.0f;
            if (UIList* layout = dynamic_cast<UIList*>(head))
            {
                float content = 0.0f;
                bool first = true;
                for (auto& child : layout->children)
                {
                    if (!child || !child->visible) continue;
                    if (!first) content += layout->spacing;
                    first = false;
                    if (UIText* text = dynamic_cast<UIText*>(child.get())) content += textAdvance(text);
                    else content += child->rect.size.x;
                }
                float width = content + layout->padding * 2.0f;
                float minimum = layout->padding * 2.0f + 8.0f;
                if (width < minimum) width = minimum;
                return width;
            }
            if (head->rect.size.x > 0.0f) return head->rect.size.x;
            return 48.0f;
        }
    }

    struct UIDockSpace::Node
    {
        bool split = false;
        bool vertical = false;
        float ratio = 0.5f;
        std::unique_ptr<Node> first;
        std::unique_ptr<Node> second;
        std::vector<std::shared_ptr<UIDockPage>> pages;
        int active = 0;
        Rect rect;
        ~Node();
    };

    UIDockSpace::Node::~Node() = default;

    float UIDockSpace::tabStrip(const Node& node) const
    {
        if (node.pages.empty()) return 0.0f;
        float headH = 0.0f;
        for (auto& page : node.pages)
        {
            float h = 28.0f;
            if (page && page->head() && page->head()->rect.size.y > 0.0f) h = page->head()->rect.size.y;
            if (h > headH) headH = h;
        }
        if (headH > node.rect.size.y) headH = node.rect.size.y;
        return headH;
    }

    UIDockPage::UIDockPage()
    {
        pickable = false;
        color = Color(0.16f, 0.16f, 0.18f, 1.0f);
        auto head = std::make_shared<UIWidget>();
        head->rect = Rect(0.0f, 0.0f, 0.0f, 28.0f);
        head->color = Color(0.24f, 0.26f, 0.32f, 1.0f);
        this->setHead(head);
    }

    UIDockPage::~UIDockPage() { this->unbindHead(); }

    void UIDockPage::setDock(UIDockSpace* space, UIDockMode mode)
    {
        mSpace = space;
        mMode = mode;
    }

    void UIDockPage::setHead(const std::shared_ptr<UIWidget>& widget)
    {
        if (mHead == widget)
        {
            this->bindHead();
            this->syncChildren();
            return;
        }
        this->unbindHead();
        mHead = widget;
        this->bindHead();
        this->syncChildren();
    }

    void UIDockPage::setBody(const std::shared_ptr<UIWidget>& widget)
    {
        if (mBody == widget) return;
        mBody = widget;
        this->syncChildren();
    }

    void UIDockPage::bindHead()
    {
        if (!mHead) return;
        mHead->pickable = true;
        mHead->onPointerPress = [this]() { this->mSuppressClick = false; this->mDragging = false; };
        mHead->onPointerDrag = [this](float x, float y, int button)
        {
            if (button != 0) return;
            this->mDragging = true;
            bool engaged = false;
            if (this->onDrag) engaged = this->onDrag(*this, x, y, button);
            if (engaged) this->mSuppressClick = true;
        };
        mHead->onPointerRelease = [this]()
        {
            if (!this->mDragging) return;
            this->mDragging = false;
            if (this->onDragEnd) this->onDragEnd(*this);
        };
        mHead->onClick = [this]()
        {
            if (this->mSuppressClick)
            {
                this->mSuppressClick = false;
                return;
            }
            if (this->mSpace) this->mSpace->activate(this);
        };
    }

    void UIDockPage::unbindHead()
    {
        if (!mHead) return;
        mHead->onPointerPress = nullptr;
        mHead->onPointerDrag = nullptr;
        mHead->onPointerRelease = nullptr;
        mHead->onClick = nullptr;
    }

    void UIDockPage::syncChildren()
    {
        children.clear();
        if (mHead) children.push_back(mHead);
        if (mBody) children.push_back(mBody);
    }

    void UIDockPage::place(const Rect& headRect, const Rect& bodyRect, bool showBody, bool tabActive)
    {
        mTabActive = tabActive;
        Rect pageRect = headRect;
        if (showBody)
        {
            Vector2 headEnd = headRect.origin + headRect.size;
            Vector2 bodyEnd = bodyRect.origin + bodyRect.size;
            Vector2 corner(minf(headRect.origin.x, bodyRect.origin.x), minf(headRect.origin.y, bodyRect.origin.y));
            Vector2 extent(maxf(headEnd.x, bodyEnd.x), maxf(headEnd.y, bodyEnd.y));
            pageRect = Rect(corner, extent - corner);
        }
        this->rect = pageRect;
        auto localOf = [&](const Rect& area)
        {
            return Rect(area.origin - pageRect.origin, area.size);
        };
        if (mHead)
        {
            mHead->visible = visible;
            mHead->pickable = true;
            mHead->rect = localOf(headRect);
            placeHeadContent(mHead.get());
        }
        if (mBody)
        {
            mBody->visible = showBody && visible;
            mBody->layout(localOf(bodyRect));
        }
    }

    void UIDockPage::layout(const Rect& rect)
    {
        float headH = 28.0f;
        if (mHead && mHead->rect.size.y > 0.0f) headH = snap(mHead->rect.size.y);
        float w = snap(rect.size.x);
        float h = snap(rect.size.y);
        if (w < 1.0f) w = 1.0f;
        if (h < 1.0f) h = 1.0f;
        if (headH > h) headH = h;
        if (headH < 0.0f) headH = 0.0f;
        float bodyH = h - headH;
        float x = snap(rect.origin.x);
        float y = snap(rect.origin.y);
        this->place(Rect(x, y, w, headH), Rect(x, y + headH, w, bodyH), true, false);
        this->rect = Rect(x, y, w, h);
    }

    void UIDockPage::layoutInWindow(float width, float height)
    {
        this->layout(Rect(0.0f, 0.0f, width, height));
    }

    void UIDockPage::render(UIPrimitive& primitive)
    {
        if (!visible) return;
        Rect solid = UIFont::solidUV();
        Color savedHead;
        bool recolor = false;
        if (mTabActive && mHead)
        {
            savedHead = mHead->color;
            mHead->color = activeFill;
            recolor = true;
        }
        primitive.pushScaleAround(rect.origin, localScale);
        primitive.pushClip(primitive.toScreen(rect));
        primitive.pushOrigin(primitive.toScreen(rect.origin));
        if (mBody && mBody->visible && color.a > 0.0f)
        {
            primitive.pushScaleAround(mBody->rect.origin, mBody->localScale);
            primitive.addQuad(mBody->rect, solid, color);
            primitive.popOrigin();
        }
        if (mHead && mHead->visible && mHead->color.a > 0.0f)
        {
            primitive.pushScaleAround(mHead->rect.origin, mHead->localScale);
            primitive.addQuad(mHead->rect, solid, mHead->color);
            primitive.popOrigin();
        }
        if (mBody && mBody->visible && !primitive.outsideClip(mBody->finalRect())) mBody->render(primitive);
        if (mHead && mHead->visible)
        {
            primitive.pushClip(primitive.toScreen(mHead->finalRect()));
            mHead->UIWidget::render(primitive);
            primitive.popClip();
            float hx = snap(mHead->rect.origin.x);
            float hy = snap(mHead->rect.origin.y);
            float hw = snap(mHead->rect.origin.x + mHead->rect.size.x) - hx;
            float hh = snap(mHead->rect.origin.y + mHead->rect.size.y) - hy;
            if (hw < 0.0f) hw = 0.0f;
            if (hh < 0.0f) hh = 0.0f;
            if (mSpace == nullptr && hh >= 1.0f)
            {
                hairline(primitive, hx, hy + hh - 1.0f, hw, false, Color(0.55f, 0.58f, 0.66f, 1.0f));
            }
        }
        primitive.popOrigin();
        primitive.popClip();
        primitive.popOrigin();
        if (recolor && mHead) mHead->color = savedHead;
    }

    UIDockSpace::UIDockSpace()
    {
        pickable = true;
        color = Color(0.14f, 0.14f, 0.16f, 1.0f);
        mRoot = std::make_unique<Node>();
    }

    UIDockSpace::~UIDockSpace()
    {
        auto callback = onDestroy;
        onDestroy = nullptr;
        if (callback) callback(*this);
        mRoot.reset();
    }

    void UIDockSpace::setScreenMapper(const std::function<Rect(const Rect&)>& mapper) { mScreenMapper = mapper; }

    Rect UIDockSpace::toScreen(const Rect& local) const
    {
        if (mScreenMapper) return mScreenMapper(local);
        return local;
    }

    void UIDockSpace::ensure(const std::shared_ptr<UIDockPage>& page)
    {
        if (!page) return;
        if (page->space() == this) return;
        if (page->space()) page->space()->takePage(page.get());
    }

    std::shared_ptr<UIDockPage> UIDockSpace::findShared(UIDockPage* page) const
    {
        for (auto& child : children)
        {
            if (child.get() == page) return std::static_pointer_cast<UIDockPage>(child);
        }
        return nullptr;
    }

    void UIDockSpace::addPage(const std::shared_ptr<UIDockPage>& page) { this->ensure(page); }

    std::shared_ptr<UIDockPage> UIDockSpace::takePage(UIDockPage* page)
    {
        auto held = this->findShared(page);
        if (page == nullptr || page->space() != this) return held;
        this->extract(mRoot, page);
        if (!mRoot) mRoot = std::make_unique<Node>();
        for (auto it = children.begin(); it != children.end(); ++it)
        {
            if (it->get() == page)
            {
                if (!held) held = std::static_pointer_cast<UIDockPage>(*it);
                children.erase(it);
                break;
            }
        }
        page->setDock(nullptr, UIDockMode::None);
        page->floating = false;
        this->reconcile(mRoot.get(), UIDockMode::Fill);
        this->clearPreview();
        return held;
    }

    void UIDockSpace::removePage(const std::shared_ptr<UIDockPage>& page)
    {
        if (!page) return;
        this->takePage(page.get());
    }

    bool UIDockSpace::extract(std::unique_ptr<Node>& node, UIDockPage* page)
    {
        if (!node) return false;
        if (!node->split)
        {
            auto& pages = node->pages;
            for (size_t i = 0; i < pages.size(); ++i)
            {
                if (pages[i].get() != page) continue;
                pages.erase(pages.begin() + (ptrdiff_t)i);
                if (pages.empty()) node->active = 0;
                else if (node->active >= (int)pages.size()) node->active = (int)pages.size() - 1;
                return true;
            }
            return false;
        }
        if (this->extract(node->first, page))
        {
            if (node->first && !node->first->split && node->first->pages.empty())
            {
                node = std::move(node->second);
            }
            return true;
        }
        if (this->extract(node->second, page))
        {
            if (node->second && !node->second->split && node->second->pages.empty())
            {
                node = std::move(node->first);
            }
            return true;
        }
        return false;
    }

    void UIDockSpace::reconcile(Node* node, UIDockMode side)
    {
        if (!node) return;
        if (!node->split)
        {
            for (auto& page : node->pages)
            {
                if (!page) continue;
                page->setDock(page->space(), (node->pages.size() > 1) ? UIDockMode::Fill : side);
            }
            return;
        }
        UIDockMode firstSide = node->vertical ? UIDockMode::Top : UIDockMode::Left;
        UIDockMode secondSide = node->vertical ? UIDockMode::Bottom : UIDockMode::Right;
        this->reconcile(node->first.get(), firstSide);
        this->reconcile(node->second.get(), secondSide);
    }

    void UIDockSpace::fillLeaf(Node* leaf, const std::shared_ptr<UIDockPage>& page)
    {
        if (!leaf || leaf->split || !page) return;
        for (auto& item : leaf->pages)
        {
            if (item == page) 
            {
                leaf->active = (int)(&item - leaf->pages.data());
                return;
            }
        }
        leaf->pages.push_back(page);
        leaf->active = (int)leaf->pages.size() - 1;
        page->setDock(this, UIDockMode::Fill);
        page->floating = false;
        bool found = false;
        for (auto& child : children) if (child == page) found = true;
        if (!found) children.push_back(page);
    }

    void UIDockSpace::splitLeaf(Node* leaf, const std::shared_ptr<UIDockPage>& page, UIDockMode mode)
    {
        if (!leaf || leaf->split || !page) return;
        auto previous = std::make_unique<Node>();
        previous->pages = std::move(leaf->pages);
        previous->active = leaf->active;
        auto incoming = std::make_unique<Node>();
        incoming->pages.push_back(page);
        incoming->active = 0;
        page->setDock(this, mode);
        page->floating = false;
        bool found = false;
        for (auto& child : children) if (child == page) found = true;
        if (!found) children.push_back(page);
        bool incomingFirst = mode == UIDockMode::Left || mode == UIDockMode::Top;
        leaf->split = true;
        leaf->vertical = mode == UIDockMode::Top || mode == UIDockMode::Bottom;
        leaf->ratio = 0.5f;
        leaf->pages.clear();
        leaf->active = 0;
        if (incomingFirst)
        {
            leaf->first = std::move(incoming);
            leaf->second = std::move(previous);
        }
        else
        {
            leaf->first = std::move(previous);
            leaf->second = std::move(incoming);
        }
    }

    void UIDockSpace::splitRoot(const std::shared_ptr<UIDockPage>& page, UIDockMode mode)
    {
        auto previous = std::move(mRoot);
        if (!previous) previous = std::make_unique<Node>();
        auto incoming = std::make_unique<Node>();
        incoming->pages.push_back(page);
        incoming->active = 0;
        page->setDock(this, mode);
        page->floating = false;
        bool found = false;
        for (auto& child : children) if (child == page) found = true;
        if (!found) children.push_back(page);
        mRoot = std::make_unique<Node>();
        mRoot->split = true;
        mRoot->vertical = mode == UIDockMode::Top || mode == UIDockMode::Bottom;
        mRoot->ratio = 0.5f;
        bool incomingFirst = mode == UIDockMode::Left || mode == UIDockMode::Top;
        if (incomingFirst)
        {
            mRoot->first = std::move(incoming);
            mRoot->second = std::move(previous);
        }
        else
        {
            mRoot->first = std::move(previous);
            mRoot->second = std::move(incoming);
        }
    }

    UIDockSpace::Node* UIDockSpace::findPageNode(Node* node, UIDockPage* page)
    {
        if (!node || !page) return nullptr;
        if (!node->split)
        {
            for (auto& item : node->pages) if (item.get() == page) return node;
            return nullptr;
        }
        if (auto* found = this->findPageNode(node->first.get(), page)) return found;
        return this->findPageNode(node->second.get(), page);
    }

    Vector2 UIDockSpace::toLocal(const Vector2& point) const
    {
        return point - rect.origin;
    }

    Vector2 UIDockSpace::pointerLocal(float x, float y) const
    {
        Vector2 delta(x - rect.origin.x, y - rect.origin.y);
        if (localScale.x == 0.0f || localScale.y == 0.0f)
        {
            return Vector2::ZERO;
        }
        return Vector2(delta.x / localScale.x, delta.y / localScale.y);
    }

    UIDockSpace::Node* UIDockSpace::findLeaf(Node* node, const Vector2& point)
    {
        if (!node) return nullptr;
        Rect area = node->rect;
        if (!area.contains(point)) return nullptr;
        if (!node->split) return node;
        if (auto* found = this->findLeaf(node->first.get(), point)) return found;
        return this->findLeaf(node->second.get(), point);
    }

    UIDockSpace::Node* UIDockSpace::nearestLeaf(Node* node, const Vector2& point)
    {
        if (!node) return nullptr;
        if (!node->split) return node;
        Node* a = this->nearestLeaf(node->first.get(), point);
        Node* b = this->nearestLeaf(node->second.get(), point);
        if (!a) return b;
        if (!b) return a;
        Vector2 ac = a->rect.origin + a->rect.size * 0.5f;
        Vector2 bc = b->rect.origin + b->rect.size * 0.5f;
        float da = (ac - point).sqrmagnitude();
        float db = (bc - point).sqrmagnitude();
        return da <= db ? a : b;
    }

    UIDockSpace::Node* UIDockSpace::firstLeaf(Node* node)
    {
        if (!node) return nullptr;
        if (!node->split) return node;
        if (auto* leaf = this->firstLeaf(node->first.get())) return leaf;
        return this->firstLeaf(node->second.get());
    }

    float UIDockSpace::bandOf(const Rect& area) const
    {
        float side = minf(area.size.x, area.size.y);
        return minf(dropBand, side * 0.5f);
    }

    float UIDockSpace::fitRatio(float ratio, float inner) const
    {
        if (inner <= splitterSize + minPane * 2.0f) return 0.5f;
        float minRatio = minPane / inner;
        float maxRatio = (inner - splitterSize - minPane) / inner;
        if (minRatio > maxRatio) return 0.5f;
        return clampf(ratio, minRatio, maxRatio);
    }

    UIDockMode UIDockSpace::zoneAt(const Rect& area, const Vector2& point) const
    {
        float band = this->bandOf(area);
        Vector2 innerMin = area.origin + Vector2(band, band);
        Vector2 innerMax = area.origin + area.size - Vector2(band, band);
        if (point.x >= innerMin.x && point.x < innerMax.x && point.y >= innerMin.y && point.y < innerMax.y) return UIDockMode::Fill;
        float left = point.x - area.origin.x;
        float right = area.origin.x + area.size.x - point.x;
        float top = point.y - area.origin.y;
        float bottom = area.origin.y + area.size.y - point.y;
        float best = left;
        UIDockMode mode = UIDockMode::Left;
        if (right < best) { best = right; mode = UIDockMode::Right; }
        if (top < best) { best = top; mode = UIDockMode::Top; }
        if (bottom < best) { best = bottom; mode = UIDockMode::Bottom; }
        (void)best;
        return mode;
    }

    Rect UIDockSpace::previewRectFor(const Rect& area, UIDockMode mode) const
    {
        if (mode == UIDockMode::Fill || mode == UIDockMode::None) return area;
        float innerW = area.size.x - splitterSize;
        float innerH = area.size.y - splitterSize;
        if (innerW < 0.0f) innerW = 0.0f;
        if (innerH < 0.0f) innerH = 0.0f;
        if (mode == UIDockMode::Left) return Rect(area.origin.x, area.origin.y, innerW * 0.5f, area.size.y);
        if (mode == UIDockMode::Right) return Rect(area.origin.x + innerW * 0.5f + splitterSize, area.origin.y, innerW * 0.5f, area.size.y);
        if (mode == UIDockMode::Top) return Rect(area.origin.x, area.origin.y, area.size.x, innerH * 0.5f);
        return Rect(area.origin.x, area.origin.y + innerH * 0.5f + splitterSize, area.size.x, innerH * 0.5f);
    }

    Rect UIDockSpace::splitterRect(Node* node) const
    {
        if (!node || !node->split || !node->first) return Rect();
        Rect area = node->rect;
        if (node->vertical)
        {
            float inner = area.size.y - splitterSize;
            if (inner < 0.0f) inner = 0.0f;
            float ratio = this->fitRatio(node->ratio, area.size.y);
            return Rect(area.origin.x, area.origin.y + inner * ratio, area.size.x, splitterSize);
        }
        float inner = area.size.x - splitterSize;
        if (inner < 0.0f) inner = 0.0f;
        float ratio = this->fitRatio(node->ratio, area.size.x);
        return Rect(area.origin.x + inner * ratio, area.origin.y, splitterSize, area.size.y);
    }

    UIDockSpace::Node* UIDockSpace::findSplitter(Node* node, const Vector2& point, float slop) const
    {
        if (!node || !node->split) return nullptr;
        if (auto* found = this->findSplitter(node->first.get(), point, slop)) return found;
        if (auto* found = this->findSplitter(node->second.get(), point, slop)) return found;
        Rect gap = this->splitterRect(node);
        if (slop > 0.0f)
        {
            if (node->vertical)
            {
                gap.origin.y -= slop;
                gap.size.y += slop * 2.0f;
            }
            else
            {
                gap.origin.x -= slop;
                gap.size.x += slop * 2.0f;
            }
        }
        if (gap.contains(point)) return node;
        return nullptr;
    }

    bool UIDockSpace::splitterAxis(float x, float y, bool& vertical) const
    {
        if (mSplitterTracking && mDragSplitter)
        {
            vertical = mDragSplitter->vertical;
            return true;
        }
        Node* hit = this->findSplitter(mRoot.get(), this->pointerLocal(x, y), 2.0f);
        if (!hit) return false;
        vertical = hit->vertical;
        return true;
    }

    void UIDockSpace::setHover(float x, float y)
    {
        mHoverSplitter = this->findSplitter(mRoot.get(), this->pointerLocal(x, y), 2.0f);
    }

    void UIDockSpace::layoutLeaf(Node* node)
    {
        if (!node) return;
        Rect area = node->rect;
        if (node->pages.empty()) return;
        float left = snap(area.origin.x);
        float top = snap(area.origin.y);
        float right = snap(area.origin.x + area.size.x);
        float bottom = snap(area.origin.y + area.size.y);
        float headH = snap(this->tabStrip(*node));
        if (top + headH > bottom) headH = bottom - top;
        if (headH < 0.0f) headH = 0.0f;
        int count = (int)node->pages.size();
        if (node->active < 0 || node->active >= count) node->active = 0;
        std::vector<float> widths((size_t)count, 0.0f);
        float total = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            auto& page = node->pages[i];
            if (!page) continue;
            float width = snap(headContentWidth(page->head().get()));
            widths[(size_t)i] = width;
            total += width;
        }
        float scale = 1.0f;
        float span = right - left;
        if (total > span && total > 0.0f) scale = span / total;
        float bodyH = bottom - top - headH;
        if (bodyH < 0.0f) bodyH = 0.0f;
        float x = left;
        for (int i = 0; i < count; ++i)
        {
            auto& page = node->pages[i];
            if (!page) continue;
            float w = snap(widths[(size_t)i] * scale);
            if (x >= right) w = 0.0f;
            else if (x + w > right) w = right - x;
            bool active = i == node->active;
            Rect head(x, top, w, headH);
            Rect body(left, top + headH, right - left, bodyH);
            page->place(head, body, active, active);
            page->visible = true;
            page->floating = false;
            x += w;
        }
    }

    void UIDockSpace::layoutNode(Node* node, const Rect& area)
    {
        if (!node) return;
        node->rect = area;
        if (!node->split)
        {
            this->layoutLeaf(node);
            return;
        }
        float span = node->vertical ? area.size.y : area.size.x;
        float ratio = this->fitRatio(node->ratio, span);
        float inner = span - splitterSize;
        if (inner < 0.0f) inner = 0.0f;
        float firstSize = inner * ratio;
        float secondSize = inner - firstSize;
        if (node->vertical)
        {
            this->layoutNode(node->first.get(), Rect(area.origin.x, area.origin.y, area.size.x, firstSize));
            this->layoutNode(node->second.get(), Rect(area.origin.x, area.origin.y + firstSize + splitterSize, area.size.x, secondSize));
        }
        else
        {
            this->layoutNode(node->first.get(), Rect(area.origin.x, area.origin.y, firstSize, area.size.y));
            this->layoutNode(node->second.get(), Rect(area.origin.x + firstSize + splitterSize, area.origin.y, secondSize, area.size.y));
        }
    }

    void UIDockSpace::layout(const Rect& rect)
    {
        this->rect = rect;
        this->layoutTree();
    }

    bool UIDockSpace::pageBounds(UIDockPage* page, Rect& bounds)
    {
        Node* leaf = this->findPageNode(mRoot.get(), page);
        if (!leaf) return false;
        bounds = Rect(rect.origin + leaf->rect.origin, leaf->rect.size);
        return true;
    }

    void UIDockSpace::layoutTree()
    {
        if (!mRoot) return;
        this->layoutNode(mRoot.get(), Rect(Vector2::ZERO, rect.size));
    }

    void UIDockSpace::dockPage(const std::shared_ptr<UIDockPage>& page, UIDockMode mode)
    {
        if (!page) return;
        this->ensure(page);
        if (mode == UIDockMode::None)
        {
            page->setDock(nullptr, UIDockMode::None);
            return;
        }
        if (!mRoot) mRoot = std::make_unique<Node>();
        if (mode == UIDockMode::Fill)
        {
            Node* leaf = this->firstLeaf(mRoot.get());
            if (mRoot->split)
            {
                Vector2 center = this->toLocal(rect.origin + rect.size * 0.5f);
                leaf = this->findLeaf(mRoot.get(), center);
                if (!leaf) leaf = this->nearestLeaf(mRoot.get(), center);
            }
            this->fillLeaf(leaf, page);
        }
        else if (!mRoot->split && mRoot->pages.empty())
        {
            this->fillLeaf(mRoot.get(), page);
            page->setDock(this, mode);
        }
        else
        {
            this->splitRoot(page, mode);
        }
        this->reconcile(mRoot.get(), UIDockMode::Fill);
        this->layout(this->rect);
    }

    void UIDockSpace::activate(UIDockPage* page)
    {
        Node* leaf = this->findPageNode(mRoot.get(), page);
        if (!leaf) return;
        for (int i = 0; i < (int)leaf->pages.size(); ++i)
        {
            if (leaf->pages[i].get() == page) leaf->active = i;
        }
        this->layout(this->rect);
    }

    bool UIDockSpace::showPreview(float localX, float localY)
    {
        this->layout(this->rect);
        Vector2 local = this->pointerLocal(localX, localY);
        Node* leaf = this->findLeaf(mRoot.get(), local);
        if (!leaf) leaf = this->nearestLeaf(mRoot.get(), local);
        if (!leaf)
        {
            this->clearPreview();
            return false;
        }
        Rect area = leaf->rect;
        if (!area.contains(local))
        {
            this->clearPreview();
            return false;
        }
        float tabH = this->tabStrip(*leaf);
        Rect zones = area;
        UIDockMode mode = UIDockMode::Fill;
        if (local.y >= area.origin.y + tabH)
        {
            zones.origin.y += tabH;
            zones.size.y -= tabH;
            if (zones.size.y < 0.0f) zones.size.y = 0.0f;
            mode = this->zoneAt(zones, local);
        }
        mPreview = true;
        mPreviewLeaf = leaf;
        mPreviewMode = mode;
        mPreviewRect = this->previewRectFor(area, mPreviewMode);
        return true;
    }

    void UIDockSpace::clearPreview()
    {
        mPreview = false;
        mPreviewMode = UIDockMode::None;
        mPreviewLeaf = nullptr;
        mPreviewRect = Rect();
    }

    void UIDockSpace::acceptDrop(const std::shared_ptr<UIDockPage>& page)
    {
        Node* leaf = mPreviewLeaf;
        UIDockMode mode = mPreviewMode;
        this->clearPreview();
        if (!page || !leaf || mode == UIDockMode::None) return;
        this->ensure(page);
        if (mode == UIDockMode::Fill) this->fillLeaf(leaf, page);
        else this->splitLeaf(leaf, page, mode);
        this->reconcile(mRoot.get(), UIDockMode::Fill);
        this->layout(this->rect);
    }

    void UIDockSpace::renderNode(Node* node, UIPrimitive& primitive)
    {
        if (!node) return;
        if (node->split)
        {
            this->renderNode(node->first.get(), primitive);
            this->renderNode(node->second.get(), primitive);
            Rect gap = this->splitterRect(node);
            bool hot = node == mDragSplitter || node == mHoverSplitter;
            Color color = hot ? splitterHover : splitter;
            primitive.addQuad(gap, UIFont::solidUV(), color);
            return;
        }
        Rect screen = primitive.toScreen(node->rect);
        primitive.pushClip(screen);
        if (node->pages.empty()) primitive.addQuad(node->rect, UIFont::solidUV(), empty);
        else
        {
            float left = snap(node->rect.origin.x);
            float top = snap(node->rect.origin.y);
            float stripW = snap(node->rect.origin.x + node->rect.size.x) - left;
            float headH = snap(this->tabStrip(*node));
            if (headH > 0.0f && stripW > 0.0f)
            {
                Color bar = empty;
                for (auto& page : node->pages)
                {
                    if (page && page->head())
                    {
                        bar = page->head()->color;
                        break;
                    }
                }
                primitive.addQuad(Rect(left, top, stripW, headH), UIFont::solidUV(), bar);
            }
        }
        for (int i = 0; i < (int)node->pages.size(); ++i)
        {
            if (i == node->active) continue;
            if (node->pages[i]) node->pages[i]->render(primitive);
        }
        if (node->active >= 0 && node->active < (int)node->pages.size() && node->pages[node->active])
        {
            node->pages[node->active]->render(primitive);
        }
        if (!node->pages.empty())
        {
            float left = snap(node->rect.origin.x);
            float top = snap(node->rect.origin.y);
            float right = snap(node->rect.origin.x + node->rect.size.x);
            float headH = snap(this->tabStrip(*node));
            if (headH >= 1.0f && right > left)
            {
                hairline(primitive, left, top + headH - 1.0f, right - left, false, border.color);
                int count = (int)node->pages.size();
                for (int i = 0; i < count; ++i)
                {
                    auto& page = node->pages[i];
                    if (!page || !page->head()) continue;
                    const Rect& tab = page->head()->rect;
                    float left = page->rect.origin.x + tab.origin.x;
                    float hx = snap(left);
                    float hw = snap(left + tab.size.x) - hx;
                    if (hw < 1.0f) continue;
                    float tabRight = hx + hw;
                    if (tabRight < right - 0.5f)
                    {
                        hairline(primitive, tabRight - 1.0f, top, headH - 1.0f, true, border.color);
                    }
                    if (i == node->active)
                    {
                        float accent = headH >= 2.0f ? 2.0f : 1.0f;
                        primitive.addQuad(Rect(hx, top + headH - accent, hw, accent), UIFont::solidUV(), page->tabMark);
                    }
                }
            }
        }
        primitive.popClip();
        UIStroke::border(primitive, node->rect, border);
    }

    void UIDockSpace::render(UIPrimitive& primitive)
    {
        if (!visible) return;
        primitive.pushScaleAround(rect.origin, localScale);
        primitive.pushClip(primitive.toScreen(rect));
        primitive.pushOrigin(primitive.toScreen(rect.origin));
        this->renderNode(mRoot.get(), primitive);
        if (mPreview && mPreviewRect.size.x > 0.0f && mPreviewRect.size.y > 0.0f)
        {
            primitive.addQuad(mPreviewRect, UIFont::solidUV(), preview);
        }
        UIStroke::border(primitive, Rect(Vector2::ZERO, rect.size), border);
        primitive.popOrigin();
        primitive.popClip();
        primitive.popOrigin();
    }

    void UIDockSpace::triggerPointerDrag(float x, float y, int button)
    {
        if (button != 0) return;
        Vector2 local = this->pointerLocal(x, y);
        if (!mSplitterTracking)
        {
            mDragSplitter = this->findSplitter(mRoot.get(), local, 0.0f);
            if (!mDragSplitter) return;
            mSplitterTracking = true;
            Rect gap = this->splitterRect(mDragSplitter);
            mSplitterGrab = mDragSplitter->vertical ? (local.y - gap.origin.y) : (local.x - gap.origin.x);
        }
        if (!mDragSplitter) return;
        Rect area = mDragSplitter->rect;
        float span = mDragSplitter->vertical ? area.size.y : area.size.x;
        float origin = mDragSplitter->vertical ? area.origin.y : area.origin.x;
        float pointer = mDragSplitter->vertical ? local.y : local.x;
        float inner = span - splitterSize;
        if (inner <= 0.0f) return;
        float first = pointer - mSplitterGrab - origin;
        mDragSplitter->ratio = this->fitRatio(first / inner, span);
        this->layout(this->rect);
    }

    void UIDockSpace::triggerPointerRelease()
    {
        mSplitterTracking = false;
        mDragSplitter = nullptr;
    }

}
