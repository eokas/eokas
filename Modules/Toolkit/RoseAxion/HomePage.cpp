#include "./HomePage.h"
#include "./Theme.h"
#include "./UiKit.h"

#include "UI/Main.h"
#include "UI/UIFont.h"

#include <cmath>
#include <utility>
#include <vector>

namespace eokas::RoseAxion {

    void RoseHomePage::bind(TableDocument& doc, const Host& host)
    {
        mDoc = &doc;
        mHost = host;
    }

    void RoseHomePage::reset()
    {
        mKeepY = 0.0f;
        mBackdrop = nullptr;
        mView = nullptr;
        this->detachChildren();
    }

    const String& RoseHomePage::font() const
    {
        return mHost.font;
    }

    float RoseHomePage::scale() const
    {
        return hostScale(mHost.scale);
    }

    void RoseHomePage::notifyRebuild()
    {
        if (mHost.requestRebuild)
        {
            mHost.requestRebuild();
        }
    }

    void RoseHomePage::goTo(EditorPage page)
    {
        if (mHost.selectPage)
        {
            mHost.selectPage(page);
        }
    }

    void RoseHomePage::openTable(u32_t index)
    {
        if (mDoc == nullptr)
        {
            return;
        }
        if (mDoc->selectTable(index))
        {
            this->goTo(EditorPage::Tables);
        }
    }

    void RoseHomePage::openSchema(u32_t index)
    {
        if (mDoc == nullptr)
        {
            return;
        }
        if (mDoc->selectSchema(index))
        {
            this->goTo(EditorPage::Schemas);
        }
    }

    void RoseHomePage::addTable()
    {
        if (mDoc == nullptr)
        {
            return;
        }
        if (mDoc->addTable())
        {
            this->goTo(EditorPage::Tables);
        }
        else
        {
            this->notifyRebuild();
        }
    }

    String RoseHomePage::documentTitle() const
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return "RoseAxion";
        }
        return fileTitleFromPath(mDoc->path());
    }

    String RoseHomePage::documentSubtitle() const
    {
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return String::empty;
        }
        if (mDoc->path().isEmpty())
        {
            return "尚未写入磁盘的 Library";
        }
        return mDoc->path();
    }

    void RoseHomePage::layout()
    {
        if (!visible)
        {
            return;
        }
        if (mBackdrop != nullptr)
        {
            setBox(*mBackdrop, Rect(0.0f, 0.0f, shape.size.x, shape.size.y));
        }
        if (mView != nullptr)
        {
            setBox(*mView, Rect(0.0f, 0.0f, shape.size.x, shape.size.y));
        }
        UIWidget::layout();
    }

    void RoseHomePage::rebuild()
    {
        if (mView != nullptr)
        {
            mKeepY = mView->scrollY();
        }
        this->detachChildren();
        mBackdrop = nullptr;
        mView = nullptr;
        const RoseTheme& t = theme();
        this->color = t.voidBg;
        this->shape.pivot = Vector2(0.0f, 0.0f);
        if (mDoc == nullptr || !mDoc->isOpen())
        {
            return;
        }

        const float s = this->scale();
        const float fontPx = 14.0f * s;
        const float smallPx = 12.0f * s;
        const float titlePx = 32.0f * s;
        const float valuePx = 34.0f * s;
        const float pad = 28.0f * s;
        const float gap = 14.0f * s;
        const float cardH = 108.0f * s;
        const float itemH = 38.0f * s;
        const float headerH = 40.0f * s;
        const float maxListBody = 300.0f * s;
        const Color ink = t.ink;
        const Color mute = t.mute;
        const Color cyan = t.accentBright;
        const Color violet = t.secondary;
        const Color amber = t.warn;
        const Color primary = t.accent;
        const Color clear(0.0f, 0.0f, 0.0f, 0.0f);
        const String& fontPath = this->font();
        const float width = shape.size.x;
        const float height = shape.size.y;
        float innerW = width - pad * 2.0f;
        if (innerW < 160.0f * s)
        {
            innerW = 160.0f * s;
        }

        auto backdrop = std::make_shared<PaintPanel>();
        backdrop->fill(t.voidBg);
        if (t.glow)
        {
            backdrop->orbs = {
                { width * 0.78f, -40.0f * s, 220.0f * s, t.orbA, 0.2f, true },
                { width * 0.92f, height * 0.42f, 180.0f * s, t.orbB, 1.7f, true },
                { -30.0f * s, height * 0.72f, 160.0f * s, t.orbC, 2.9f, true },
            };
        }
        setBox(backdrop, Rect(0.0f, 0.0f, width, height));
        this->attachChild(backdrop);
        mBackdrop = backdrop.get();

        auto view = std::make_shared<UIView>();
        view->shape.pivot = Vector2(0.0f, 0.0f);
        view->color = clear;
        setBox(view, Rect(0.0f, 0.0f, width, height));

        auto content = std::make_shared<UIWidget>();
        content->shape.pivot = Vector2(0.0f, 0.0f);
        content->pickable = false;
        content->color = clear;

        auto hero = std::make_shared<PaintPanel>();
        hero->fill(t.paper);
        hero->hair = t.hair;
        hero->framed = true;
        hero->underline = withAlpha(t.accentBright, 0.85f);
        hero->underlineH = 2.0f * s;
        if (t.glow)
        {
            hero->orbs = {
                { innerW * 0.82f, 18.0f * s, 130.0f * s, withAlpha(t.accentBright, 0.28f), 0.4f, true },
                { innerW * 0.96f, 110.0f * s, 90.0f * s, withAlpha(t.secondary, 0.22f), 1.8f, true },
                { innerW * 0.08f, 130.0f * s, 70.0f * s, withAlpha(t.secondary, 0.16f), 2.4f, true },
            };
        }

        auto markA = std::make_shared<UIWidget>();
        markA->pickable = false;
        markA->color = cyan;
        setBox(markA, Rect(0.0f, 0.0f, 10.0f * s, 10.0f * s));
        auto markB = std::make_shared<UIWidget>();
        markB->pickable = false;
        markB->color = violet;
        setBox(markB, Rect(6.0f * s, 6.0f * s, 10.0f * s, 10.0f * s));
        auto brand = makeText("ROSEAXION", fontPath, 11.0f * s, cyan, innerW, 16.0f * s);
        brand->layout();

        auto title = makeText(this->documentTitle(), fontPath, titlePx, ink, innerW, 40.0f * s);
        title->layout();
        auto sub = makeText(this->documentSubtitle(), fontPath, smallPx, mute, innerW, 20.0f * s);
        sub->layout();

        auto statusRow = std::make_shared<UIList>();
        statusRow->direction = UIDirection::Horizontal;
        statusRow->padding = 0.0f;
        statusRow->spacing = 8.0f * s;
        statusRow->centerCross = true;
        statusRow->color = clear;
        statusRow->shape.pivot = Vector2(0.0f, 0.0f);
        if (mDoc->path().isEmpty())
        {
            statusRow->attachChild(makeChip("LIVE SESSION", fontPath, 11.0f * s, withAlpha(cyan, t.washAlpha), cyan));
        }
        else
        {
            statusRow->attachChild(makeChip("LIBRARY", fontPath, 11.0f * s, withAlpha(violet, t.washAlpha), violet));
        }
        if (mDoc->dirty())
        {
            statusRow->attachChild(makeChip("未保存", fontPath, 11.0f * s, withAlpha(amber, t.washAlpha), amber));
        }
        setBox(statusRow, Rect(0.0f, 0.0f, 0.0f, 0.0f));
        statusRow->layout();

        auto openTables = makeButton("打开表格", fontPath, fontPx, 118.0f * s, 36.0f * s, primary, t.inkOnAccent);
        openTables->onClick = [this]() { this->goTo(EditorPage::Tables); };
        auto openSchemas = makeButton("打开模式", fontPath, fontPx, 118.0f * s, 36.0f * s, t.ghost, ink);
        openSchemas->hoverFill = withAlpha(cyan, t.washAlpha);
        openSchemas->pressedFill = withAlpha(cyan, t.washAlpha * 1.6f);
        openSchemas->onClick = [this]() { this->goTo(EditorPage::Schemas); };
        auto createTable = makeButton("新建表格", fontPath, fontPx, 118.0f * s, 36.0f * s, t.ghost, ink);
        createTable->hoverFill = withAlpha(violet, t.washAlpha);
        createTable->pressedFill = withAlpha(violet, t.washAlpha * 1.6f);
        createTable->onClick = [this]() { this->addTable(); };
        auto actions = std::make_shared<UIList>();
        actions->direction = UIDirection::Horizontal;
        actions->padding = 0.0f;
        actions->spacing = 10.0f * s;
        actions->centerCross = true;
        actions->color = clear;
        actions->shape.pivot = Vector2(0.0f, 0.0f);
        actions->attachChild(openTables);
        actions->attachChild(openSchemas);
        actions->attachChild(createTable);
        setBox(actions, Rect(0.0f, 0.0f, 0.0f, 0.0f));
        actions->layout();

        const float heroPad = 22.0f * s;
        float y = heroPad;
        setBox(markA, Rect(heroPad, y + 2.0f * s, 10.0f * s, 10.0f * s));
        setBox(markB, Rect(heroPad + 6.0f * s, y + 8.0f * s, 10.0f * s, 10.0f * s));
        setBox(brand, Rect(heroPad + 22.0f * s, y, brand->shape.size.x, brand->shape.size.y));
        hero->attachChild(markA);
        hero->attachChild(markB);
        hero->attachChild(brand);
        y += 22.0f * s;
        float titleW = title->shape.size.x;
        if (titleW > innerW - heroPad * 2.0f)
        {
            titleW = innerW - heroPad * 2.0f;
        }
        setBox(title, Rect(heroPad, y, titleW, title->shape.size.y));
        hero->attachChild(title);
        y += title->shape.size.y + 6.0f * s;
        setBox(sub, Rect(heroPad, y, sub->shape.size.x, sub->shape.size.y));
        hero->attachChild(sub);
        y += sub->shape.size.y + 10.0f * s;
        setBox(statusRow, Rect(heroPad, y, statusRow->shape.size.x, statusRow->shape.size.y));
        hero->attachChild(statusRow);
        y += statusRow->shape.size.y + 16.0f * s;
        setBox(actions, Rect(heroPad, y, actions->shape.size.x, actions->shape.size.y));
        hero->attachChild(actions);
        y += actions->shape.size.y + heroPad;
        setBox(hero, Rect(0.0f, 0.0f, innerW, y));
        content->attachChild(hero);
        float cursorY = y + 18.0f * s;

        const int statCols = innerW >= 560.0f * s ? 3 : 1;
        float statCellW = innerW;
        if (statCols > 1)
        {
            statCellW = (innerW - gap * (float)(statCols - 1)) / (float)statCols;
        }
        if (statCellW < 80.0f * s)
        {
            statCellW = 80.0f * s;
        }

        auto makeStat = [&](const String& label, const String& value, const Color& accent, std::function<void()> action)
        {
            auto box = std::make_shared<PaintPanel>();
            box->pickable = true;
            box->hoverLift = true;
            box->fill(t.paper);
            box->accent = accent;
            box->accentW = 4.0f * s;
            box->hair = t.hair;
            box->framed = true;
            setBox(box, Rect(0.0f, 0.0f, statCellW, cardH));
            box->onClick = std::move(action);
            const float inset = 18.0f * s;
            float textW = statCellW - inset * 2.0f;
            if (textW < 24.0f * s)
            {
                textW = 24.0f * s;
            }
            auto caption = makeText(label, fontPath, smallPx, mute, textW, 18.0f * s);
            caption->layout();
            setBox(caption, Rect(inset, 16.0f * s, caption->shape.size.x, caption->shape.size.y));
            auto number = makeText(value, fontPath, valuePx, ink, textW, 40.0f * s);
            number->layout();
            setBox(number, Rect(inset, 16.0f * s + caption->shape.size.y + 8.0f * s, number->shape.size.x, number->shape.size.y));
            box->attachChild(caption);
            box->attachChild(number);
            return box;
        };

        const u32_t tableCount = mDoc->tableCount();
        const u32_t schemaCount = mDoc->userSchemaCount();
        const u32_t rowCount = mDoc->rowCount();
        auto stats = std::make_shared<UIGrid>();
        stats->columns = statCols;
        stats->padding = 0.0f;
        stats->spacing = gap;
        stats->cellHeight = cardH;
        stats->color = clear;
        stats->shape.pivot = Vector2(0.0f, 0.0f);
        setBox(stats, Rect(0.0f, 0.0f, innerW, 0.0f));
        stats->attachChild(makeStat("TABLES", String::format("%u", tableCount), cyan, [this]() { this->goTo(EditorPage::Tables); }));
        stats->attachChild(makeStat("SCHEMAS", String::format("%u", schemaCount), violet, [this]() { this->goTo(EditorPage::Schemas); }));
        stats->attachChild(makeStat("ROWS", String::format("%u", rowCount), amber, [this]() { this->goTo(EditorPage::Tables); }));
        stats->layout();
        setBox(stats, Rect(0.0f, cursorY, innerW, stats->shape.size.y));
        content->attachChild(stats);
        cursorY += stats->shape.size.y + 20.0f * s;

        auto makeSection = [&](const char* heading, const Color& accent, float sectionW, u32_t count, bool hasActive, u32_t active,
            const std::function<String(u32_t)>& nameOf, const std::function<String(u32_t)>& extraOf,
            const std::function<Color(u32_t)>& extraColorOf, const std::function<void(u32_t)>& openOf, const char* emptyHint)
        {
            auto section = std::make_shared<PaintPanel>();
            section->fill(t.paper);
            section->hair = t.hair;
            section->framed = true;

            const float textX = 16.0f * s;
            auto head = std::make_shared<UIWidget>();
            head->shape.pivot = Vector2(0.0f, 0.0f);
            head->pickable = false;
            head->color = t.header;
            setBox(head, Rect(0.0f, 0.0f, sectionW, headerH));
            auto dot = std::make_shared<UIWidget>();
            dot->pickable = false;
            dot->color = accent;
            setBox(dot, Rect(textX, (headerH - 6.0f * s) * 0.5f, 6.0f * s, 6.0f * s));
            head->attachChild(dot);
            auto headTitle = makeText(heading, fontPath, fontPx, ink, sectionW - 80.0f * s, 22.0f * s);
            headTitle->layout();
            setBox(headTitle, Rect(textX + 12.0f * s, (headerH - headTitle->shape.size.y) * 0.5f, headTitle->shape.size.x, headTitle->shape.size.y));
            head->attachChild(headTitle);
            auto countChip = makeChip(String::format("%u", count), fontPath, 11.0f * s, withAlpha(accent, t.washAlpha), accent);
            float countX = sectionW - 14.0f * s - countChip->shape.size.x;
            if (countX < 30.0f * s)
            {
                countX = 30.0f * s;
            }
            setBox(countChip, Rect(countX, (headerH - countChip->shape.size.y) * 0.5f, countChip->shape.size.x, countChip->shape.size.y));
            head->attachChild(countChip);
            auto headRule = makeRule(sectionW, t.line);
            setBox(headRule, Rect(0.0f, headerH - 1.0f, sectionW, 1.0f));
            head->attachChild(headRule);
            section->attachChild(head);

            auto list = std::make_shared<UIList>();
            list->direction = UIDirection::Vertical;
            list->padding = 0.0f;
            list->spacing = 0.0f;
            list->color = clear;
            list->shape.pivot = Vector2(0.0f, 0.0f);

            if (count == 0)
            {
                auto hint = makeText(emptyHint, fontPath, smallPx, mute, sectionW - textX * 2.0f, 20.0f * s);
                hint->layout();
                auto empty = std::make_shared<UIWidget>();
                empty->shape.pivot = Vector2(0.0f, 0.0f);
                empty->pickable = false;
                empty->color = clear;
                setBox(empty, Rect(0.0f, 0.0f, sectionW, itemH));
                setBox(hint, Rect(textX, (itemH - hint->shape.size.y) * 0.5f, hint->shape.size.x, hint->shape.size.y));
                empty->attachChild(hint);
                list->attachChild(empty);
            }
            else
            {
                const float rowW = sectionW;
                for (u32_t index = 0; index < count; ++index)
                {
                    const bool on = count > 1 && hasActive && active == index;
                    auto item = std::make_shared<PaintPanel>();
                    item->pickable = true;
                    item->hoverLift = true;
                    if (on)
                    {
                        item->fill(t.selected);
                    }
                    else
                    {
                        item->fill(t.paper);
                    }
                    setBox(item, Rect(0.0f, 0.0f, rowW, itemH));
                    item->onClick = [openOf, index]() { openOf(index); };

                    String extra = extraOf(index);
                    float extraW = 0.0f;
                    if (!extra.isEmpty())
                    {
                        Color extraColor = extraColorOf(index);
                        auto chip = makeChip(extra, fontPath, 11.0f * s, withAlpha(extraColor, t.washAlpha), extraColor);
                        extraW = chip->shape.size.x + 10.0f * s;
                        setBox(chip, Rect(rowW - chip->shape.size.x - 14.0f * s, (itemH - chip->shape.size.y) * 0.5f, chip->shape.size.x, chip->shape.size.y));
                        item->attachChild(chip);
                    }
                    float nameW = rowW - textX - 14.0f * s - extraW;
                    if (nameW < 24.0f * s)
                    {
                        nameW = 24.0f * s;
                    }
                    auto name = makeText(nameOf(index), fontPath, fontPx, ink, nameW, 22.0f * s);
                    name->layout();
                    float nameH = name->shape.size.y;
                    if (nameH < 1.0f)
                    {
                        nameH = 20.0f * s;
                    }
                    setBox(name, Rect(textX, (itemH - nameH) * 0.5f, nameW, nameH));
                    item->attachChild(name);
                    list->attachChild(item);
                }
            }
            setBox(list, Rect(0.0f, 0.0f, sectionW, 0.0f));
            list->layout();

            float bodyH = list->shape.size.y;
            if (bodyH < itemH)
            {
                bodyH = itemH;
            }
            if (bodyH > maxListBody)
            {
                auto scroller = std::make_shared<UIView>();
                scroller->shape.pivot = Vector2(0.0f, 0.0f);
                scroller->color = clear;
                setBox(scroller, Rect(0.0f, headerH, sectionW, maxListBody));
                scroller->addChild(list);
                section->attachChild(scroller);
                bodyH = maxListBody;
            }
            else
            {
                setBox(list, Rect(0.0f, headerH, sectionW, bodyH));
                section->attachChild(list);
            }
            setBox(section, Rect(0.0f, 0.0f, sectionW, headerH + bodyH));
            return section;
        };

        const bool stack = innerW < 640.0f * s;
        const float colGap = 16.0f * s;
        float colW = stack ? innerW : floorf((innerW - colGap) * 0.5f);
        if (colW < 160.0f * s)
        {
            colW = innerW;
        }
        auto tables = makeSection("Tables", cyan, colW, tableCount, mDoc->hasActiveTable(), mDoc->activeTableIndex(),
            [this](u32_t index) { return mDoc->tableName(index); },
            [this, tableCount](u32_t index) -> String
            {
                if (tableCount > 1 && mDoc->hasActiveTable() && mDoc->activeTableIndex() == index)
                {
                    return "ACTIVE";
                }
                return String::empty;
            },
            [cyan](u32_t) { return cyan; },
            [this](u32_t index) { this->openTable(index); },
            "还没有表格");
        auto schemas = makeSection("Schemas", violet, colW, schemaCount, mDoc->hasActiveSchema(), mDoc->activeSchemaIndex(),
            [this](u32_t index) { return mDoc->schemaName(index); },
            [this](u32_t index) { return String(schemaTypeLabel(mDoc->schemaType(index))); },
            [this, cyan, violet](u32_t index)
            {
                return mDoc->schemaType(index) == rose::SchemaType::List ? cyan : violet;
            },
            [this](u32_t index) { this->openSchema(index); },
            "还没有模式");

        if (stack || colW >= innerW)
        {
            setBox(tables, Rect(0.0f, cursorY, colW, tables->shape.size.y));
            content->attachChild(tables);
            cursorY += tables->shape.size.y + colGap;
            setBox(schemas, Rect(0.0f, cursorY, colW, schemas->shape.size.y));
            content->attachChild(schemas);
            cursorY += schemas->shape.size.y;
        }
        else
        {
            float rowH = tables->shape.size.y;
            if (schemas->shape.size.y > rowH)
            {
                rowH = schemas->shape.size.y;
            }
            setBox(tables, Rect(0.0f, cursorY, colW, tables->shape.size.y));
            setBox(schemas, Rect(colW + colGap, cursorY, colW, schemas->shape.size.y));
            content->attachChild(tables);
            content->attachChild(schemas);
            cursorY += rowH;
        }

        setBox(content, Rect(pad, pad, innerW, cursorY));
        view->addChild(content);
        view->setScroll(0.0f, mKeepY);
        this->attachChild(view);
        mView = view.get();
        this->layout();
    }

}
