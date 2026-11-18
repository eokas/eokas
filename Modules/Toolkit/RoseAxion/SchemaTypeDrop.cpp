#include "./SchemaTypeDrop.h"
#include "./FieldTypes.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"
#include "UI/UIStroke.h"

#include <cmath>

namespace eokas::RoseAxion {

    void RoseSchemaTypeDrop::layout()
    {
        if (!visible)
        {
            return;
        }
        if (children().empty() || !children()[0])
        {
            return;
        }
        UIWidget& label = *children()[0];
        label.layout();
        float y = (shape.size.y - label.shape.size.y) * 0.5f;
        if (y < 0.0f)
        {
            y = 0.0f;
        }
        this->placeChild(label, Vector2(floorf(paddingX + 0.5f), floorf(y + 0.5f)));
    }

    void RoseSchemaTypeDrop::render(UIPrimitive& primitive)
    {
        if (!visible)
        {
            return;
        }
        Matrix3 localToScreen = this->matrixLocalToScreen();
        Color bg = field;
        if (this->isPressed())
        {
            bg = pressedFill;
        }
        else if (this->isHovered() || open)
        {
            bg = hoverFill;
        }
        primitive.addQuad(localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), UIFont::solidUV(), bg);
        UIStroke::border(primitive, localToScreen, Rect(0.0f, 0.0f, shape.size.x, shape.size.y), border);
        float cs = shape.size.y * 0.28f;
        if (cs > 8.0f)
        {
            cs = 8.0f;
        }
        if (cs < 4.0f)
        {
            cs = 4.0f;
        }
        float cx = shape.size.x - paddingX - cs * 0.5f;
        float cy = shape.size.y * 0.5f;
        Rect uv = UIFont::solidUV();
        if (open)
        {
            primitive.addQuad(
                this->localToScreen(Vector2(cx - cs * 0.5f, cy + cs * 0.25f)),
                this->localToScreen(Vector2(cx + cs * 0.5f, cy + cs * 0.25f)),
                this->localToScreen(Vector2(cx, cy - cs * 0.35f)),
                this->localToScreen(Vector2(cx, cy - cs * 0.35f)),
                uv,
                chevron);
        }
        else
        {
            primitive.addQuad(
                this->localToScreen(Vector2(cx - cs * 0.5f, cy - cs * 0.25f)),
                this->localToScreen(Vector2(cx + cs * 0.5f, cy - cs * 0.25f)),
                this->localToScreen(Vector2(cx, cy + cs * 0.35f)),
                this->localToScreen(Vector2(cx, cy + cs * 0.35f)),
                uv,
                chevron);
        }
        UIWidget::render(primitive);
    }

    void RoseSchemaTypeMenu::close(UIWidget* overlay)
    {
        if (overlay != nullptr && mMenu != nullptr)
        {
            overlay->detachChild(mMenu);
        }
        mMenu = nullptr;
        mTypeAnchor = nullptr;
        mTypeSlot = -2;
    }

    bool RoseSchemaTypeMenu::visible() const
    {
        return mMenu != nullptr && mMenu->visible;
    }

    void RoseSchemaTypeMenu::place(UIWidget* overlay)
    {
        if (overlay == nullptr || mMenu == nullptr || !mMenu->visible || mTypeAnchor == nullptr)
        {
            return;
        }
        placePopup(*overlay, *mMenu, mTypeAnchor->screenBounds());
    }

    void RoseSchemaTypeMenu::open(
        UIWidget& anchor,
        int typeSlot,
        TableDocument& doc,
        const RoseUiHost& host,
        float scale,
        UIWidget* overlay,
        const std::function<void(const String&)>& onPick)
    {
        if (overlay == nullptr)
        {
            return;
        }
        const float s = hostScale(scale);
        const String& fontPath = host.font;
        auto entries = schemaAssignableTypeEntries(doc, onPick);
        auto menu = std::make_shared<UIMenu>();
        menu->direction = UIDirection::Vertical;
        stylePopupMenu(*menu, s);
        menu->overlay = true;
        menu->shape.pivot = Vector2(0.0f, 0.0f);
        menu->host = &anchor;
        for (const auto& entry : entries)
        {
            auto item = std::make_shared<UIMenuItem>();
            item->setText(entry.first);
            styleMenuItem(*item, fontPath, menuFontPx(s), s);
            auto action = entry.second;
            UIMenu* menuRaw = menu.get();
            item->onClick = [menuRaw, action]()
            {
                if (menuRaw != nullptr)
                {
                    menuRaw->visible = false;
                }
                action();
            };
            menu->addItem(item);
        }
        overlay->attachChild(menu);
        mMenu = menu.get();
        mTypeAnchor = &anchor;
        mTypeSlot = typeSlot;
        placePopup(*overlay, *menu, anchor.screenBounds());
    }

    std::vector<std::pair<String, std::function<void()>>> schemaAssignableTypeEntries(
        TableDocument& doc,
        const std::function<void(const String&)>& onPick)
    {
        std::vector<std::pair<String, std::function<void()>>> entries;
        std::vector<String> names = doc.schemaAssignableTypeNames();
        for (const String& name : names)
        {
            entries.push_back({ schemaLabel(name), [onPick, name]() { onPick(name); } });
        }
        return entries;
    }

    RoseSchemaTypeDrop* attachSchemaTypeDrop(
        UIWidget& row,
        const String& typeName,
        int typeSlot,
        int activeSlot,
        float x,
        float y,
        float dropW,
        float fieldH,
        float scale,
        const String& fontPath,
        float smallPx,
        const Color& ink,
        const Color& mute,
        const RoseTheme& t,
        TableDocument& doc,
        const RoseUiHost& host,
        UIWidget* overlay,
        RoseSchemaTypeMenu& menu,
        const std::function<void(const String&)>& onPick,
        const std::function<void()>& onOpenMenu)
    {
        const float s = hostScale(scale);
        const Color field = t.field;
        auto drop = std::make_shared<RoseSchemaTypeDrop>();
        drop->shape.pivot = Vector2(0.0f, 0.0f);
        drop->field = field;
        drop->hoverFill = hoverOf(field);
        drop->pressedFill = pressOf(field);
        drop->border.color = t.ring;
        drop->border.thickness = 1.0f;
        drop->chevron = mute;
        drop->paddingX = 8.0f * s;
        drop->open = activeSlot == typeSlot;
        setBox(drop, Rect(x, y, dropW, fieldH));
        String caption = schemaLabel(typeName);
        if (caption.isEmpty())
        {
            caption = schemaLabel(String("String"));
        }
        auto label = makeText(caption, fontPath, smallPx, ink, dropW - 28.0f * s, 20.0f * s);
        label->pickable = false;
        drop->attachChild(label);
        RoseSchemaTypeDrop* raw = drop.get();
        drop->onClick = [raw, typeSlot, &menu, &doc, host, scale, overlay, onPick, onOpenMenu]()
        {
            onOpenMenu();
            raw->open = true;
            menu.open(*raw, typeSlot, doc, host, scale, overlay, onPick);
        };
        row.attachChild(drop);
        if (activeSlot == typeSlot)
        {
            menu.setAnchor(raw);
        }
        return raw;
    }

}