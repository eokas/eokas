#ifndef _EOKAS_ROSE_AXION_SCHEMA_TYPE_DROP_H_
#define _EOKAS_ROSE_AXION_SCHEMA_TYPE_DROP_H_

#include "./Document.h"
#include "./Host.h"
#include "./Theme.h"

#include "UI/UIWidget.h"

#include <functional>
#include <utility>
#include <vector>

namespace eokas::RoseAxion {

    class RoseSchemaTypeDrop : public UIWidget
    {
    public:
        Color field;
        Color hoverFill;
        Color pressedFill;
        UIStrokeStyle border;
        Color chevron;
        bool open = false;
        float paddingX = 8.0f;

        void layout() override;
        void render(UIPrimitive& primitive) override;
    };

    class RoseSchemaTypeMenu
    {
    public:
        void close(UIWidget* overlay);
        bool visible() const;
        int activeSlot() const { return mTypeSlot; }
        UIWidget* menu() const { return mMenu; }
        UIWidget* anchor() const { return mTypeAnchor; }
        void setAnchor(UIWidget* anchor) { mTypeAnchor = anchor; }
        void place(UIWidget* overlay);
        void open(
            UIWidget& anchor,
            int typeSlot,
            TableDocument& doc,
            const RoseUiHost& host,
            float scale,
            UIWidget* overlay,
            const std::function<void(const String&)>& onPick);
    private:
        UIWidget* mMenu = nullptr;
        UIWidget* mTypeAnchor = nullptr;
        int mTypeSlot = -2;
    };

    std::vector<std::pair<String, std::function<void()>>> schemaAssignableTypeEntries(
        TableDocument& doc,
        const std::function<void(const String&)>& onPick);

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
        const RoseTheme& theme,
        TableDocument& doc,
        const RoseUiHost& host,
        UIWidget* overlay,
        RoseSchemaTypeMenu& menu,
        const std::function<void(const String&)>& onPick,
        const std::function<void()>& onOpenMenu);

}

#endif