#include "UIDocking.h"
#include "UIFont.h"
#include "UILayout.h"
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

        void strokeRect(UIShape& shape, const Rect& area, float thickness, const Color& color)
        {
            if (thickness <= 0.0f || area.width <= 0.0f || area.height <= 0.0f) return;
            float t = thickness;
            if (t * 2.0f > area.width) t = area.width * 0.5f;
            if (t * 2.0f > area.height) t = area.height * 0.5f;
            Rect solid = UIFont::solidUV();
            shape.addQuad(Rect(area.x, area.y, area.width, t), solid, color);
            shape.addQuad(Rect(area.x, area.y + area.height - t, area.width, t), solid, color);
            shape.addQuad(Rect(area.x, area.y, t, area.height), solid, color);
            shape.addQuad(Rect(area.x + area.width - t, area.y, t, area.height), solid, color);
        }

        float snap(float value)
        {
            return floorf(value + 0.5f);
        }

        void textMetrics(UIText* text, float& scale, float& ascender, float& descender)
        {
            scale = 1.0f;
            ascender = 0.0f;
            descender = 0.0f;
            if (!text || !text->font || !text->font->isOpen()) return;
            text->font->drawMetrics(text->fontSize, scale, ascender, descender);
        }

        float textAdvance(UIText* text)
        {
            if (!text) return 0.0f;
            if (text->font == nullptr || !text->font->isOpen()) return text->rect.width;
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
                width += text->font->glyphSized(codepoint, text->fontSize).advance * scale;
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
            if (line < 1.0f && text) line = text->rect.height;
            return line;
        }

        void hairline(UIShape& shape, float x, float y, float length, bool vertical, const Color& color)
        {
            float sx = snap(x);
            float sy = snap(y);
            float span = snap(length);
            if (span < 1.0f) return;
            Rect solid = UIFont::solidUV();
            if (vertical) shape.addQuad(Rect(sx, sy, 1.0f, span), solid, color);
            else shape.addQuad(Rect(sx, sy, span, 1.0f), solid, color);
        }

        void placeHeadContent(UIWidget* head)
        {
            if (!head) return;
            float padX = 8.0f;
            float spacing = 0.0f;
            if (UILayout* layout = dynamic_cast<UILayout*>(head))
            {
                padX = layout->padding;
                spacing = layout->spacing;
            }
            padX = snap(padX);
            float x = snap(head->rect.x + padX);
            float right = snap(head->rect.x + head->rect.width) - padX;
            bool first = true;
            for (auto& child : head->children)
            {
                if (!child || !child->visible) continue;
                if (!first) x += spacing;
                first = false;
                float w = child->rect.width;
                float h = child->rect.height;
                if (UIText* text = dynamic_cast<UIText*>(child.get()))
                {
                    w = ceilf(textAdvance(text));
                    h = textLine(text);
                    if (w < 1.0f) w = 1.0f;
                    text->rect.width = w;
                    text->rect.height = h;
                }
                if (right > x && x + w > right) w = right - x;
                float y = head->rect.y + (head->rect.height - h) * 0.5f;
                if (y < head->rect.y) y = head->rect.y;
                child->rect.x = snap(x);
                child->rect.y = snap(y);
                child->rect.width = w;
                x += w;
            }
        }

        float headContentWidth(UIWidget* head)
        {
            if (!head) return 48.0f;
            if (UILayout* layout = dynamic_cast<UILayout*>(head))
            {
                float content = 0.0f;
                bool first = true;
                for (auto& child : layout->children)
                {
                    if (!child || !child->visible) continue;
                    if (!first) content += layout->spacing;
                    first = false;
                    if (UIText* text = dynamic_cast<UIText*>(child.get())) content += textAdvance(text);
                    else content += child->rect.width;
                }
                float width = content + layout->padding * 2.0f;
                float minimum = layout->padding * 2.0f + 8.0f;
                if (width < minimum) width = minimum;
                return width;
            }
            if (head->rect.width > 0.0f) return head->rect.width;
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
            if (page && page->head() && page->head()->rect.height > 0.0f) h = page->head()->rect.height;
            if (h > headH) headH = h;
        }
        if (headH > node.rect.height) headH = node.rect.height;
        return headH;
    }

    UIDockPage::UIDockPage()
    {
        interactive = false;
        color = { 0.16f, 0.16f, 0.18f, 1.0f };
        auto head = std::make_shared<UIWidget>();
        head->rect = Rect(0.0f, 0.0f, 0.0f, 28.0f);
        head->color = { 0.24f, 0.26f, 0.32f, 1.0f };
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
        mHead->interactive = true;
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
        if (mHead)
        {
            mHead->visible = visible;
            mHead->interactive = true;
            mHead->rect = headRect;
        }
        if (mBody)
        {
            mBody->visible = showBody && visible;
            mBody->rect = bodyRect;
        }
        if (!showBody)
        {
            rect = headRect;
            return;
        }
        float x0 = headRect.x;
        float y0 = headRect.y;
        float x1 = headRect.x + headRect.width;
        float y1 = headRect.y + headRect.height;
        if (mBody)
        {
            x0 = minf(x0, bodyRect.x);
            y0 = minf(y0, bodyRect.y);
            x1 = maxf(x1, bodyRect.x + bodyRect.width);
            y1 = maxf(y1, bodyRect.y + bodyRect.height);
        }
        rect = Rect(x0, y0, x1 - x0, y1 - y0);
    }

    void UIDockPage::layoutInWindow(float width, float height)
    {
        float headH = 28.0f;
        if (mHead && mHead->rect.height > 0.0f) headH = snap(mHead->rect.height);
        float w = snap(width);
        float h = snap(height);
        if (w < 1.0f) w = 1.0f;
        if (h < 1.0f) h = 1.0f;
        if (headH > h) headH = h;
        if (headH < 0.0f) headH = 0.0f;
        float bodyH = h - headH;
        this->place(Rect(0.0f, 0.0f, w, headH), Rect(0.0f, headH, w, bodyH), true, false);
        rect = Rect(0.0f, 0.0f, w, h);
    }

    void UIDockPage::render(UIShape& shape)
    {
        if (!visible) return;
        Rect solid = UIFont::solidUV();
        Color savedHead;
        bool recolor = false;
        if (mTabActive && mHead)
        {
            savedHead = mHead->color;
            mHead->color = activeColor;
            recolor = true;
        }
        if (mBody && mBody->visible && color.a > 0.0f) shape.addQuad(mBody->rect, solid, color);
        if (mHead && mHead->visible && mHead->color.a > 0.0f) shape.addQuad(mHead->rect, solid, mHead->color);
        if (mBody && mBody->visible && !shape.outsideClip(mBody->rect)) mBody->render(shape);
        if (mHead && mHead->visible)
        {
            placeHeadContent(mHead.get());
            Vector2 origin = shape.offset();
            float hx = snap(mHead->rect.x);
            float hy = snap(mHead->rect.y);
            float hw = snap(mHead->rect.x + mHead->rect.width) - hx;
            float hh = snap(mHead->rect.y + mHead->rect.height) - hy;
            if (hw < 0.0f) hw = 0.0f;
            if (hh < 0.0f) hh = 0.0f;
            Rect clip(origin.x + hx, origin.y + hy, hw, hh);
            shape.pushClip(clip);
            mHead->UIWidget::render(shape);
            shape.popClip();
            if (mSpace == nullptr && hh >= 1.0f)
            {
                hairline(shape, hx, hy + hh - 1.0f, hw, false, Color(0.55f, 0.58f, 0.66f, 1.0f));
            }
        }
        if (recolor && mHead) mHead->color = savedHead;
    }

    UIDockSpace::UIDockSpace()
    {
        interactive = true;
        color = { 0.14f, 0.14f, 0.16f, 1.0f };
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

    UIDockSpace::Node* UIDockSpace::findLeaf(Node* node, float x, float y)
    {
        if (!node) return nullptr;
        Rect area = node->rect;
        if (!area.contains(Vector2(x, y))) return nullptr;
        if (!node->split) return node;
        if (auto* found = this->findLeaf(node->first.get(), x, y)) return found;
        return this->findLeaf(node->second.get(), x, y);
    }

    UIDockSpace::Node* UIDockSpace::nearestLeaf(Node* node, float x, float y)
    {
        if (!node) return nullptr;
        if (!node->split) return node;
        Node* a = this->nearestLeaf(node->first.get(), x, y);
        Node* b = this->nearestLeaf(node->second.get(), x, y);
        if (!a) return b;
        if (!b) return a;
        float ax = a->rect.x + a->rect.width * 0.5f;
        float ay = a->rect.y + a->rect.height * 0.5f;
        float bx = b->rect.x + b->rect.width * 0.5f;
        float by = b->rect.y + b->rect.height * 0.5f;
        float da = (ax - x) * (ax - x) + (ay - y) * (ay - y);
        float db = (bx - x) * (bx - x) + (by - y) * (by - y);
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
        float side = minf(area.width, area.height);
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

    UIDockMode UIDockSpace::zoneAt(const Rect& area, float x, float y) const
    {
        float band = this->bandOf(area);
        float cx0 = area.x + band;
        float cy0 = area.y + band;
        float cx1 = area.x + area.width - band;
        float cy1 = area.y + area.height - band;
        if (x >= cx0 && x < cx1 && y >= cy0 && y < cy1) return UIDockMode::Fill;
        float left = x - area.x;
        float right = area.x + area.width - x;
        float top = y - area.y;
        float bottom = area.y + area.height - y;
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
        float innerW = area.width - splitterSize;
        float innerH = area.height - splitterSize;
        if (innerW < 0.0f) innerW = 0.0f;
        if (innerH < 0.0f) innerH = 0.0f;
        if (mode == UIDockMode::Left) return Rect(area.x, area.y, innerW * 0.5f, area.height);
        if (mode == UIDockMode::Right) return Rect(area.x + innerW * 0.5f + splitterSize, area.y, innerW * 0.5f, area.height);
        if (mode == UIDockMode::Top) return Rect(area.x, area.y, area.width, innerH * 0.5f);
        return Rect(area.x, area.y + innerH * 0.5f + splitterSize, area.width, innerH * 0.5f);
    }

    Rect UIDockSpace::splitterRect(Node* node) const
    {
        if (!node || !node->split || !node->first) return Rect();
        Rect area = node->rect;
        if (node->vertical)
        {
            float inner = area.height - splitterSize;
            if (inner < 0.0f) inner = 0.0f;
            float ratio = this->fitRatio(node->ratio, area.height);
            return Rect(area.x, area.y + inner * ratio, area.width, splitterSize);
        }
        float inner = area.width - splitterSize;
        if (inner < 0.0f) inner = 0.0f;
        float ratio = this->fitRatio(node->ratio, area.width);
        return Rect(area.x + inner * ratio, area.y, splitterSize, area.height);
    }

    UIDockSpace::Node* UIDockSpace::findSplitter(Node* node, float x, float y, float slop) const
    {
        if (!node || !node->split) return nullptr;
        if (auto* found = this->findSplitter(node->first.get(), x, y, slop)) return found;
        if (auto* found = this->findSplitter(node->second.get(), x, y, slop)) return found;
        Rect gap = this->splitterRect(node);
        if (slop > 0.0f)
        {
            if (node->vertical)
            {
                gap.y -= slop;
                gap.height += slop * 2.0f;
            }
            else
            {
                gap.x -= slop;
                gap.width += slop * 2.0f;
            }
        }
        if (gap.contains(Vector2(x, y))) return node;
        return nullptr;
    }

    bool UIDockSpace::splitterAxis(float x, float y, bool& vertical) const
    {
        if (mSplitterTracking && mDragSplitter)
        {
            vertical = mDragSplitter->vertical;
            return true;
        }
        Node* hit = this->findSplitter(mRoot.get(), x, y, 2.0f);
        if (!hit) return false;
        vertical = hit->vertical;
        return true;
    }

    void UIDockSpace::setHover(float x, float y)
    {
        mHoverSplitter = this->findSplitter(mRoot.get(), x, y, 2.0f);
    }

    void UIDockSpace::layoutLeaf(Node* node)
    {
        if (!node) return;
        Rect area = node->rect;
        if (node->pages.empty()) return;
        float left = snap(area.x);
        float top = snap(area.y);
        float right = snap(area.x + area.width);
        float bottom = snap(area.y + area.height);
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
        float span = node->vertical ? area.height : area.width;
        float ratio = this->fitRatio(node->ratio, span);
        float inner = span - splitterSize;
        if (inner < 0.0f) inner = 0.0f;
        float firstSize = inner * ratio;
        float secondSize = inner - firstSize;
        if (node->vertical)
        {
            this->layoutNode(node->first.get(), Rect(area.x, area.y, area.width, firstSize));
            this->layoutNode(node->second.get(), Rect(area.x, area.y + firstSize + splitterSize, area.width, secondSize));
        }
        else
        {
            this->layoutNode(node->first.get(), Rect(area.x, area.y, firstSize, area.height));
            this->layoutNode(node->second.get(), Rect(area.x + firstSize + splitterSize, area.y, secondSize, area.height));
        }
    }

    void UIDockSpace::layout() { this->layoutTree(); }

    bool UIDockSpace::pageBounds(UIDockPage* page, Rect& bounds)
    {
        Node* leaf = this->findPageNode(mRoot.get(), page);
        if (!leaf) return false;
        bounds = leaf->rect;
        return true;
    }

    void UIDockSpace::layoutTree()
    {
        if (!mRoot) return;
        this->layoutNode(mRoot.get(), rect);
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
                float cx = rect.x + rect.width * 0.5f;
                float cy = rect.y + rect.height * 0.5f;
                leaf = this->findLeaf(mRoot.get(), cx, cy);
                if (!leaf) leaf = this->nearestLeaf(mRoot.get(), cx, cy);
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
        this->layoutTree();
    }

    void UIDockSpace::activate(UIDockPage* page)
    {
        Node* leaf = this->findPageNode(mRoot.get(), page);
        if (!leaf) return;
        for (int i = 0; i < (int)leaf->pages.size(); ++i)
        {
            if (leaf->pages[i].get() == page) leaf->active = i;
        }
        this->layoutTree();
    }

    bool UIDockSpace::showPreview(float localX, float localY)
    {
        this->layoutTree();
        Node* leaf = this->findLeaf(mRoot.get(), localX, localY);
        if (!leaf) leaf = this->nearestLeaf(mRoot.get(), localX, localY);
        if (!leaf)
        {
            this->clearPreview();
            return false;
        }
        Rect area = leaf->rect;
        if (!area.contains(Vector2(localX, localY)))
        {
            this->clearPreview();
            return false;
        }
        float tabH = this->tabStrip(*leaf);
        Rect zones = area;
        UIDockMode mode = UIDockMode::Fill;
        if (localY >= area.y + tabH)
        {
            zones.y += tabH;
            zones.height -= tabH;
            if (zones.height < 0.0f) zones.height = 0.0f;
            mode = this->zoneAt(zones, localX, localY);
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
        this->layoutTree();
    }

    void UIDockSpace::renderNode(Node* node, UIShape& shape, const Vector2& origin)
    {
        if (!node) return;
        if (node->split)
        {
            this->renderNode(node->first.get(), shape, origin);
            this->renderNode(node->second.get(), shape, origin);
            Rect gap = this->splitterRect(node);
            bool hot = node == mDragSplitter || node == mHoverSplitter;
            Color color = hot ? splitterHoverColor : splitterColor;
            shape.addQuad(gap, UIFont::solidUV(), color);
            return;
        }
        Rect screen(origin.x + node->rect.x, origin.y + node->rect.y, node->rect.width, node->rect.height);
        shape.pushClip(screen);
        if (node->pages.empty()) shape.addQuad(node->rect, UIFont::solidUV(), emptyColor);
        else
        {
            float left = snap(node->rect.x);
            float top = snap(node->rect.y);
            float stripW = snap(node->rect.x + node->rect.width) - left;
            float headH = snap(this->tabStrip(*node));
            if (headH > 0.0f && stripW > 0.0f)
            {
                Color bar = emptyColor;
                for (auto& page : node->pages)
                {
                    if (page && page->head())
                    {
                        bar = page->head()->color;
                        break;
                    }
                }
                shape.addQuad(Rect(left, top, stripW, headH), UIFont::solidUV(), bar);
            }
        }
        for (int i = 0; i < (int)node->pages.size(); ++i)
        {
            if (i == node->active) continue;
            if (node->pages[i]) node->pages[i]->render(shape);
        }
        if (node->active >= 0 && node->active < (int)node->pages.size() && node->pages[node->active])
        {
            node->pages[node->active]->render(shape);
        }
        if (!node->pages.empty())
        {
            float left = snap(node->rect.x);
            float top = snap(node->rect.y);
            float right = snap(node->rect.x + node->rect.width);
            float headH = snap(this->tabStrip(*node));
            if (headH >= 1.0f && right > left)
            {
                hairline(shape, left, top + headH - 1.0f, right - left, false, borderColor);
                int count = (int)node->pages.size();
                for (int i = 0; i < count; ++i)
                {
                    auto& page = node->pages[i];
                    if (!page || !page->head()) continue;
                    const Rect& tab = page->head()->rect;
                    float hx = snap(tab.x);
                    float hw = snap(tab.x + tab.width) - hx;
                    if (hw < 1.0f) continue;
                    float tabRight = hx + hw;
                    if (tabRight < right - 0.5f)
                    {
                        hairline(shape, tabRight - 1.0f, top, headH - 1.0f, true, borderColor);
                    }
                    if (i == node->active)
                    {
                        float accent = headH >= 2.0f ? 2.0f : 1.0f;
                        shape.addQuad(Rect(hx, top + headH - accent, hw, accent), UIFont::solidUV(), page->tabMark);
                    }
                }
            }
        }
        shape.popClip();
        strokeRect(shape, node->rect, 1.0f, borderColor);
    }

    void UIDockSpace::render(UIShape& shape)
    {
        if (!visible) return;
        this->layoutTree();
        Vector2 origin = shape.offset();
        Rect screen(origin.x + rect.x, origin.y + rect.y, rect.width, rect.height);
        shape.pushClip(screen);
        this->renderNode(mRoot.get(), shape, origin);
        if (mPreview && mPreviewRect.width > 0.0f && mPreviewRect.height > 0.0f)
        {
            shape.addQuad(mPreviewRect, UIFont::solidUV(), previewColor);
        }
        strokeRect(shape, rect, 1.0f, borderColor);
        shape.popClip();
    }

    void UIDockSpace::triggerPointerDrag(float x, float y, int button)
    {
        if (button != 0) return;
        if (!mSplitterTracking)
        {
            mDragSplitter = this->findSplitter(mRoot.get(), x, y, 0.0f);
            if (!mDragSplitter) return;
            mSplitterTracking = true;
            Rect gap = this->splitterRect(mDragSplitter);
            mSplitterGrab = mDragSplitter->vertical ? (y - gap.y) : (x - gap.x);
        }
        if (!mDragSplitter) return;
        Rect area = mDragSplitter->rect;
        float span = mDragSplitter->vertical ? area.height : area.width;
        float origin = mDragSplitter->vertical ? area.y : area.x;
        float pointer = mDragSplitter->vertical ? y : x;
        float inner = span - splitterSize;
        if (inner <= 0.0f) return;
        float first = pointer - mSplitterGrab - origin;
        mDragSplitter->ratio = this->fitRatio(first / inner, span);
        this->layoutTree();
    }

    void UIDockSpace::triggerPointerRelease()
    {
        mSplitterTracking = false;
        mDragSplitter = nullptr;
    }

    UIDockHost::~UIDockHost() { this->closeAll(); }

    void UIDockHost::registerSpace(UIDockSpace* space)
    {
        if (!space) return;
        for (auto* item : mSpaces) if (item == space) return;
        space->onDestroy = [this](UIDockSpace& closing) { this->unregisterSpace(&closing); };
        mSpaces.push_back(space);
    }

    void UIDockHost::unregisterSpace(UIDockSpace* space)
    {
        for (auto it = mSpaces.begin(); it != mSpaces.end(); ++it)
        {
            if (*it != space) continue;
            mSpaces.erase(it);
            break;
        }
        if (mPreviewSpace == space) mPreviewSpace = nullptr;
        if (mSourceSpace == space) mSourceSpace = nullptr;
    }

    void UIDockHost::observe(const std::shared_ptr<UIDockPage>& page)
    {
        if (!page) return;
        page->onDrag = [this](UIDockPage& target, float x, float y, int button)
        {
            return this->dragPage(&target, x, y, button);
        };
        page->onDragEnd = [this](UIDockPage& target) { this->releasePage(&target); };
    }

    UIDockHost::Slot* UIDockHost::slotOf(UIDockPage* page)
    {
        for (auto& slot : mFloating) if (slot.page.get() == page) return &slot;
        return nullptr;
    }

    void UIDockHost::destroySlot(Slot& slot)
    {
        void* window = slot.window;
        slot.window = nullptr;
        slot.page.reset();
        if (window && onDestroyFloatingWindow) onDestroyFloatingWindow(window);
    }

    void UIDockHost::closeAll()
    {
        mDragging = false;
        mDragPage = nullptr;
        mSourceSpace = nullptr;
        mPreviewSpace = nullptr;
        for (auto& slot : mFloating) this->destroySlot(slot);
        mFloating.clear();
    }

    void UIDockHost::toScreenPoint(UIDockPage* page, float x, float y, float& screenX, float& screenY)
    {
        if (mSourceSpace)
        {
            Rect screen = mSourceSpace->toScreen(Rect(x, y, 0.0f, 0.0f));
            screenX = screen.x;
            screenY = screen.y;
            return;
        }
        if (Slot* slot = this->slotOf(page))
        {
            screenX = slot->screenRect.x + x;
            screenY = slot->screenRect.y + y;
            return;
        }
        screenX = x;
        screenY = y;
    }

    void UIDockHost::beginFloat(UIDockPage* page, float screenX, float screenY)
    {
        if (!page) return;
        if (Slot* slot = this->slotOf(page))
        {
            mGrabScreenX = screenX - slot->screenRect.x;
            mGrabScreenY = screenY - slot->screenRect.y;
            return;
        }
        Rect window(screenX, screenY, 320.0f, 240.0f);
        std::shared_ptr<UIDockPage> held;
        if (page->space())
        {
            UIDockSpace* space = page->space();
            space->layout();
            Rect local;
            if (space->pageBounds(page, local)) window = space->toScreen(local);
            held = space->takePage(page);
        }
        if (!held) return;
        if (onPrepareContent) onPrepareContent();
        void* windowHandle = nullptr;
        if (onCreateFloatingWindow) windowHandle = onCreateFloatingWindow(held, window);
        mGrabScreenX = screenX - window.x;
        mGrabScreenY = screenY - window.y;
        mFloating.push_back(Slot{ held, windowHandle, window });
        if (windowHandle && onPlaceFloatingWindow) onPlaceFloatingWindow(windowHandle, window);
    }

    void UIDockHost::updateFloat(UIDockPage* page, float screenX, float screenY)
    {
        Slot* slot = this->slotOf(page);
        if (!slot) return;
        slot->screenRect.x = screenX - mGrabScreenX;
        slot->screenRect.y = screenY - mGrabScreenY;
        if (slot->window && onPlaceFloatingWindow) onPlaceFloatingWindow(slot->window, slot->screenRect);
        UIDockSpace* hit = nullptr;
        for (auto* space : mSpaces)
        {
            if (!space) continue;
            Rect screen = space->toScreen(space->rect);
            if (!screen.contains(Vector2(screenX, screenY))) continue;
            float localX = space->rect.x + (screenX - screen.x);
            float localY = space->rect.y + (screenY - screen.y);
            if (space->showPreview(localX, localY)) hit = space;
            else space->clearPreview();
        }
        if (mPreviewSpace && mPreviewSpace != hit) mPreviewSpace->clearPreview();
        mPreviewSpace = hit;
    }

    bool UIDockHost::dragPage(UIDockPage* page, float x, float y, int button)
    {
        if (button != 0 || !page) return false;
        if (!mDragging || mDragPage != page)
        {
            mDragging = true;
            mDragPage = page;
            mDragMoved = false;
            mSourceSpace = page->space();
            mDragSlop = mSourceSpace ? mSourceSpace->dragSlop : 4.0f;
            this->toScreenPoint(page, x, y, mPressScreenX, mPressScreenY);
        }
        float screenX = 0.0f;
        float screenY = 0.0f;
        this->toScreenPoint(page, x, y, screenX, screenY);
        float dx = screenX - mPressScreenX;
        float dy = screenY - mPressScreenY;
        if (!mDragMoved && (dx * dx + dy * dy) < mDragSlop * mDragSlop) return false;
        if (!mDragMoved) this->beginFloat(page, screenX, screenY);
        mDragMoved = true;
        this->updateFloat(page, screenX, screenY);
        return true;
    }

    void UIDockHost::releasePage(UIDockPage* page)
    {
        if (mDragPage != page) return;
        mDragging = false;
        mDragPage = nullptr;
        mSourceSpace = nullptr;
        Slot* slot = this->slotOf(page);
        if (mDragMoved && mPreviewSpace && slot)
        {
            auto held = slot->page;
            void* window = slot->window;
            UIDockSpace* space = mPreviewSpace;
            mPreviewSpace = nullptr;
            for (auto it = mFloating.begin(); it != mFloating.end(); ++it)
            {
                if (it->page.get() != page) continue;
                mFloating.erase(it);
                break;
            }
            space->acceptDrop(held);
            if (onPrepareContent) onPrepareContent();
            if (window && onDestroyFloatingWindow) onDestroyFloatingWindow(window);
        }
        else if (mPreviewSpace)
        {
            mPreviewSpace->clearPreview();
            mPreviewSpace = nullptr;
        }
        mDragMoved = false;
    }
}
