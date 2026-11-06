#include "Unit.h"
#include "UI/Main.h"
#include "UIHost.h"
#include <cmath>

using namespace eokas;

namespace
{
    float boxLeft(const UIShape& shape)
    {
        return shape.origin.x - shape.pivot.x * shape.size.x;
    }

    float boxTop(const UIShape& shape)
    {
        return shape.origin.y - shape.pivot.y * shape.size.y;
    }

    Vector2 pivotSpace(const UIShape& shape, const Vector2& local)
    {
        return local - Vector2(shape.pivot.x * shape.size.x, shape.pivot.y * shape.size.y);
    }

    UIPointerEvent pointerEvent(float x, float y, UIPointerCode code = UIPointerCode::Left)
    {
        UIPointerEvent event;
        event.pointerCode = code;
        event.position = Vector2(x, y);
        return event;
    }
}

EOKAS_TEST_CASE(ui) {
    int clicks = 0;
    UIWidget widget;
    widget.shape.size = (Rect(0, 0, 40, 20)).size;
    widget.shape.origin = (Rect(0, 0, 40, 20)).origin + widget.shape.pivot * widget.shape.size;
    widget.onClick = [&]() { clicks += 1; };
    widget.handlePointerPress(pointerEvent(0.0f, 0.0f));
    widget.handlePointerRelease(pointerEvent(0.0f, 0.0f));
    widget.handleClick(0.0f, 0.0f);
    EOKAS_EXPECT(clicks == 1);
    widget.handlePointerEnter(pointerEvent(0.0f, 0.0f));
    EOKAS_EXPECT(widget.isHovered());
    widget.handlePointerLeave(pointerEvent(0.0f, 0.0f));
    EOKAS_EXPECT(!widget.isHovered());

    UIList list;
    auto first = std::make_shared<UIWidget>();
    auto second = std::make_shared<UIWidget>();
    first->shape.size = (Rect(0, 0, 10, 20)).size;
    first->shape.origin = (Rect(0, 0, 10, 20)).origin + first->shape.pivot * first->shape.size;
    second->shape.size = (Rect(0, 0, 10, 30)).size;
    second->shape.origin = (Rect(0, 0, 10, 30)).origin + second->shape.pivot * second->shape.size;
    list.attachChild(first);
    list.attachChild(second);
    list.shape.setBox(Vector2(0, 0), Vector2(100, 200));
    UIPrimitive listPrim;
    list.layout();
    listPrim.begin();
    list.render(listPrim);
    listPrim.end();
    EOKAS_EXPECT(_FloatEqual(boxLeft(first->shape), -42.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(first->shape), -92.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(second->shape), -64.0f));
    list.shape.setBox(Vector2(40.0f, 70.0f), Vector2(100.0f, 200.0f));
    list.layout();
    listPrim.begin();
    list.render(listPrim);
    listPrim.end();
    EOKAS_EXPECT(_FloatEqual(boxLeft(first->shape), -42.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(first->shape), -92.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(second->shape), -64.0f));

    UIList rowList;
    rowList.direction = UIDirection::Horizontal;
    rowList.centerCross = true;
    rowList.padding = 0.0f;
    rowList.spacing = 0.0f;
    auto rowItem = std::make_shared<UIWidget>();
    rowItem->shape.size = Vector2(10.0f, 20.0f);
    rowList.attachChild(rowItem);
    rowList.shape.setBox(Vector2(0.0f, 0.0f), Vector2(100.0f, 40.0f));
    rowList.layout();
    EOKAS_EXPECT(_FloatEqual(boxTop(rowItem->shape), -10.0f));

    UIGrid grid;
    grid.columns = 2;
    grid.cellWidth = 16.0f;
    grid.cellHeight = 10.0f;
    auto cell = std::make_shared<UIWidget>();
    cell->shape.size = (Rect(0, 0, 16, 10)).size;
    cell->shape.origin = (Rect(0, 0, 16, 10)).origin + cell->shape.pivot * cell->shape.size;
    grid.attachChild(cell);
    grid.shape.setBox(Vector2(0, 0), Vector2(80, 40));
    UIPrimitive gridPrim;
    grid.layout();
    gridPrim.begin();
    grid.render(gridPrim);
    gridPrim.end();
    EOKAS_EXPECT(_FloatEqual(cell->shape.size.x, 16.0f));

    UITable table;
    table.border = UITableBorder(1.0f, Color(1, 1, 1, 1));
    table.cellSpacing = 8.0f;
    table.cellPadding = 4.0f;
    table.addColumn(40.0f);
    table.addColumn(0.0f);
    table.addRow(20.0f);
    table.addRow(0.0f);
    auto body = std::make_shared<UIWidget>();
    auto side = std::make_shared<UIWidget>();
    body->shape.size = (Rect(0, 0, 12, 8)).size;
    body->shape.origin = (Rect(0, 0, 12, 8)).origin + body->shape.pivot * body->shape.size;
    side->shape.size = (Rect(0, 0, 10, 6)).size;
    side->shape.origin = (Rect(0, 0, 10, 6)).origin + side->shape.pivot * side->shape.size;
    table.setContent(1, 0, body);
    table.setContent(0, 1, side);
    table.refit();
    EOKAS_EXPECT(_FloatEqual(table.shape.size.x, 68.0f));
    EOKAS_EXPECT(_FloatEqual(table.shape.size.y, 46.0f));
    table.shape.setBox(Vector2(0, 0), Vector2(0, 0));
    UIPrimitive tablePrim;
    table.layout();
    tablePrim.begin();
    table.render(tablePrim);
    tablePrim.end();
    EOKAS_EXPECT(_FloatEqual(boxLeft(body->shape), -15.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(body->shape), -4.0f));
    table.shape.setBox(Vector2(0, 0), Vector2(100, 0));
    table.layout();
    tablePrim.begin();
    table.render(tablePrim);
    tablePrim.end();
    EOKAS_EXPECT(_FloatEqual(boxLeft(table.cell(0, 1)->shape), -2.0f));
    EOKAS_EXPECT(_FloatEqual(table.cell(0, 1)->shape.size.x, 52.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(side->shape), -21.0f));
    table.insertColumn(0, 10.0f);
    EOKAS_EXPECT(table.getContent(1, 1).get() == body.get());
    EOKAS_EXPECT(table.getContent(1, 0) == nullptr);
    table.removeRow(0);
    EOKAS_EXPECT(table.rowCount() == 1);
    EOKAS_EXPECT(table.getContent(0, 1).get() == body.get());
    EOKAS_EXPECT(table.cell(-1, 0) == nullptr);

    UITable tree;
    tree.border = UITableBorder(0.0f, Color(1, 1, 1, 1));
    tree.cellSpacing = 0.0f;
    tree.cellPadding = 0.0f;
    tree.addColumn(80.0f);
    tree.addColumn(40.0f);
    tree.setColumnIndent(0, -4.0f);
    EOKAS_EXPECT(_FloatEqual(tree.columnIndent(0), 0.0f));
    tree.setColumnIndent(0, 10.0f);
    UITableRow* group = tree.addRow(20.0f);
    UITableRow* child = group->addRow(20.0f);
    UITableRow* grand = child->addRow(20.0f);
    EOKAS_EXPECT(group->depth() == 0);
    EOKAS_EXPECT(child->ownerRow() == group);
    EOKAS_EXPECT(child->depth() == 1);
    EOKAS_EXPECT(child->index() == 0);
    EOKAS_EXPECT(grand->depth() == 2);
    auto mark = std::make_shared<UIWidget>();
    auto other = std::make_shared<UIWidget>();
    mark->shape.size = (Rect(0, 0, 12, 8)).size;
    mark->shape.origin = (Rect(0, 0, 12, 8)).origin + mark->shape.pivot * mark->shape.size;
    other->shape.size = (Rect(0, 0, 6, 6)).size;
    other->shape.origin = (Rect(0, 0, 6, 6)).origin + other->shape.pivot * other->shape.size;
    child->setContent(0, mark);
    child->setContent(1, other);
    tree.shape.setBox(Vector2(0, 0), Vector2(0, 0));
    UIPrimitive treePrim;
    tree.layout();
    treePrim.begin();
    tree.render(treePrim);
    treePrim.end();
    EOKAS_EXPECT(_FloatEqual(tree.shape.size.x, 120.0f));
    EOKAS_EXPECT(_FloatEqual(tree.shape.size.y, 60.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(mark->shape), -20.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(mark->shape), -10.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(other->shape), -20.0f));
    int leafHits = 0;
    grand->onExpandedChanged = [&](bool) { leafHits += 1; };
    grand->handlePointerPress(pointerEvent(0.0f, 0.0f));
    grand->handlePointerRelease(pointerEvent(0.0f, 0.0f));
    EOKAS_EXPECT(grand->expanded());
    EOKAS_EXPECT(leafHits == 0);
    child->setExpanded(false);
    EOKAS_EXPECT(!child->expanded());
    EOKAS_EXPECT(_FloatEqual(tree.shape.size.y, 40.0f));
    EOKAS_EXPECT(tree.children().size() == 2);
    child->setExpanded(true);
    group->setExpanded(false);
    EOKAS_EXPECT(child->expanded());
    EOKAS_EXPECT(_FloatEqual(tree.shape.size.y, 20.0f));
    EOKAS_EXPECT(tree.children().size() == 1);
    group->setExpanded(true);
    int hits = 0;
    group->onExpandedChanged = [&](bool) { hits += 1; };
    group->handlePointerPress(pointerEvent(0.0f, 0.0f));
    group->handlePointerRelease(pointerEvent(0.0f, 0.0f));
    EOKAS_EXPECT(!group->expanded());
    EOKAS_EXPECT(hits == 1);
    group->setExpanded(true);
    tree.insertColumn(0, 8.0f);
    EOKAS_EXPECT(child->getContent(1).get() == mark.get());
    EOKAS_EXPECT(child->getContent(0) == nullptr);
    EOKAS_EXPECT(_FloatEqual(tree.columnIndent(0), 0.0f));
    EOKAS_EXPECT(_FloatEqual(tree.columnIndent(1), 10.0f));
    tree.removeRow(0);
    EOKAS_EXPECT(tree.rowCount() == 0);

    UITable wide;
    wide.border = UITableBorder(0.0f, Color(1, 1, 1, 1));
    wide.addColumn(0.0f);
    wide.setColumnIndent(0, 10.0f);
    UITableRow* only = wide.addRow(0.0f);
    auto box = std::make_shared<UIWidget>();
    box->shape.size = (Rect(0, 0, 12, 8)).size;
    box->shape.origin = (Rect(0, 0, 12, 8)).origin + box->shape.pivot * box->shape.size;
    only->setContent(0, box);
    wide.refit();
    EOKAS_EXPECT(_FloatEqual(wide.shape.size.x, 22.0f));
    EOKAS_EXPECT(_FloatEqual(wide.shape.size.y, 8.0f));

    UIButton button;
    button.setText("OK");
    EOKAS_EXPECT(button.label() != nullptr);
    EOKAS_EXPECT(button.label()->text == "OK");

    int closeHits = 0;
    int clickHits = 0;
    UITag tag;
    tag.caption().text = "Alpha";
    tag.caption().shape.size = Vector2(30.0f, 12.0f);
    tag.onClose = [&]() { closeHits += 1; };
    tag.onClick = [&]() { clickHits += 1; };
    tag.layout();
    EOKAS_EXPECT(tag.caption().text == "Alpha");
    Vector2 closeScreen = tag.localToScreen(Vector2(tag.shape.size.x - 2.0f, tag.shape.size.y * 0.5f));
    EOKAS_EXPECT(tag.handleClick(closeScreen.x, closeScreen.y));
    EOKAS_EXPECT(closeHits == 1);
    EOKAS_EXPECT(clickHits == 0);
    Vector2 captionScreen = tag.localToScreen(Vector2(tag.paddingX + 2.0f, tag.shape.size.y * 0.5f));
    EOKAS_EXPECT(tag.handleClick(captionScreen.x, captionScreen.y));
    EOKAS_EXPECT(clickHits == 1);
    EOKAS_EXPECT(closeHits == 1);

    UIMenu menu;
    auto item = std::make_shared<UIMenuItem>();
    item->setText("File");
    menu.addItem(item);
    EOKAS_EXPECT(item->label() != nullptr);
    EOKAS_EXPECT(item->label()->text == "File");

    int expandedHits = 0;
    UIRegion region;
    region.onExpandedChanged = [&](bool expanded) { expandedHits += expanded ? 1 : -1; };
    region.setExpanded(false);
    EOKAS_EXPECT(!region.expanded());
    EOKAS_EXPECT(expandedHits == -1);

    UISlider slider;
    slider.setRange(0.0f, 10.0f);
    slider.setValue(12.0f);
    EOKAS_EXPECT(_FloatEqual(slider.value, 10.0f));

    int toggleHits = 0;
    UIToggle toggle;
    toggle.onValueChanged = [&](bool) { toggleHits += 1; };
    toggle.handlePointerPress(pointerEvent(0.0f, 0.0f));
    toggle.handlePointerRelease(pointerEvent(0.0f, 0.0f));
    EOKAS_EXPECT(toggle.value);
    EOKAS_EXPECT(toggleHits == 1);

    UIInput input;
    input.setText("eokas");
    EOKAS_EXPECT(input.text == "eokas");
    UIWindow inputWindow;
    inputWindow.init(80, 40);
    auto field = std::make_shared<UIInput>();
    field->shape.setBox(Vector2(0, 0), Vector2(80, 40));
    inputWindow.setRoot(field);
    inputWindow.onMouseDown(8, 8, UIPointerCode::Left);
    EOKAS_EXPECT(inputWindow.focus() == field.get());

    UIDropdown dropdown;
    dropdown.addItem(1, "One");
    dropdown.addItem(2, "Two");
    dropdown.setValue(2);
    EOKAS_EXPECT(dropdown.value == 2);
    EOKAS_EXPECT(dropdown.labelOf(2) == "Two");

    UINumberPicker numbers;
    numbers.setRange(0, 10);
    numbers.setValue(12);
    EOKAS_EXPECT(numbers.value == 10);
    numbers.setValue(-3);
    EOKAS_EXPECT(numbers.value == 0);
    numbers.wrap = true;
    numbers.setRange(0, 23);
    numbers.setValue(24);
    EOKAS_EXPECT(numbers.value == 0);
    numbers.setValue(23);
    numbers.step(1, true);
    EOKAS_EXPECT(numbers.value == 0);
    numbers.step(-1, true);
    EOKAS_EXPECT(numbers.value == 23);
    int numberHits = 0;
    int numberCommits = 0;
    numbers.onValueChanged = [&](int) { numberHits += 1; };
    numbers.onValueCommitted = [&](int) { numberCommits += 1; };
    numbers.setValue(5);
    EOKAS_EXPECT(numbers.value == 5);
    EOKAS_EXPECT(numberHits == 1);
    numbers.step(2, true);
    EOKAS_EXPECT(numbers.value == 7);
    EOKAS_EXPECT(numberHits == 2);
    EOKAS_EXPECT(numberCommits == 1);
    numbers.shape.setBox(Vector2(0.0f, 0.0f), Vector2(80.0f, 180.0f));
    numbers.handleWheel(40.0f, 90.0f, 0.0f, -48.0f);
    EOKAS_EXPECT(numbers.value == 8);

    Vector2 numberV = UINumberPicker::preferredSize(1.0f);
    EOKAS_EXPECT(_FloatEqual(numberV.x, 52.0f));
    EOKAS_EXPECT(_FloatEqual(numberV.y, 180.0f));
    Vector2 numberH = UINumberPicker::preferredSize(1.0f, UIDirection::Horizontal);
    EOKAS_EXPECT(_FloatEqual(numberH.x, 180.0f));
    EOKAS_EXPECT(_FloatEqual(numberH.y, 52.0f));

    UINumberPicker stepper;
    stepper.setRange(0, 10);
    stepper.setValue(5);
    stepper.shape.setBox(Vector2(0.0f, 0.0f), numberV);
    stepper.handleClick(numberV.x * 0.5f, 8.0f);
    EOKAS_EXPECT(stepper.value == 4);
    stepper.handleClick(numberV.x * 0.5f, numberV.y - 8.0f);
    EOKAS_EXPECT(stepper.value == 5);
    stepper.direction = UIDirection::Horizontal;
    stepper.shape.setBox(Vector2(0.0f, 0.0f), numberH);
    stepper.handleClick(8.0f, numberH.y * 0.5f);
    EOKAS_EXPECT(stepper.value == 4);
    stepper.handleClick(numberH.x - 8.0f, numberH.y * 0.5f);
    EOKAS_EXPECT(stepper.value == 5);

    UITimePicker timePicker;
    timePicker.setTime(25, 99, -1);
    EOKAS_EXPECT(timePicker.hour() == 23);
    EOKAS_EXPECT(timePicker.minute() == 59);
    EOKAS_EXPECT(timePicker.second() == 0);
    timePicker.setTime(18, 7, 9);
    EOKAS_EXPECT(timePicker.hour() == 18);
    EOKAS_EXPECT(timePicker.minute() == 7);
    EOKAS_EXPECT(timePicker.second() == 9);
    timePicker.showSeconds = false;
    EOKAS_EXPECT(timePicker.second() == 0);

    UIDatePicker datePicker;
    EOKAS_EXPECT(UIDatePicker::daysInMonth(2024, 2) == 29);
    EOKAS_EXPECT(UIDatePicker::daysInMonth(2023, 2) == 28);
    EOKAS_EXPECT(UIDatePicker::daysInMonth(1900, 2) == 28);
    EOKAS_EXPECT(UIDatePicker::daysInMonth(2000, 2) == 29);
    datePicker.setDate(2024, 2, 29);
    EOKAS_EXPECT(datePicker.year() == 2024);
    EOKAS_EXPECT(datePicker.month() == 2);
    EOKAS_EXPECT(datePicker.day() == 29);
    datePicker.setDate(2023, 2, 29);
    EOKAS_EXPECT(datePicker.year() == 2023);
    EOKAS_EXPECT(datePicker.month() == 2);
    EOKAS_EXPECT(datePicker.day() == 28);
    datePicker.setDate(2024, 1, 31);
    datePicker.setDate(2024, 2, 31);
    EOKAS_EXPECT(datePicker.day() == 29);

    UIDateTimePicker dateTime;
    dateTime.setDateTime(2026, 11, 7, 13, 30, 45);
    EOKAS_EXPECT(dateTime.year() == 2026);
    EOKAS_EXPECT(dateTime.month() == 11);
    EOKAS_EXPECT(dateTime.day() == 7);
    EOKAS_EXPECT(dateTime.hour() == 13);
    EOKAS_EXPECT(dateTime.minute() == 30);
    EOKAS_EXPECT(dateTime.second() == 45);
    TimePoint stamp = dateTime.value();
    EOKAS_EXPECT(stamp.year() == 2026);
    EOKAS_EXPECT(stamp.month() == 11);
    EOKAS_EXPECT(stamp.date() == 7);
    EOKAS_EXPECT(stamp.hour() == 13);
    EOKAS_EXPECT(stamp.minute() == 30);
    EOKAS_EXPECT(stamp.second() == 45);
    dateTime.setValue(TimePoint(2020, 2, 29, 23, 59, 59));
    EOKAS_EXPECT(dateTime.year() == 2020);
    EOKAS_EXPECT(dateTime.month() == 2);
    EOKAS_EXPECT(dateTime.day() == 29);
    EOKAS_EXPECT(dateTime.hour() == 23);
    EOKAS_EXPECT(dateTime.minute() == 59);
    EOKAS_EXPECT(dateTime.second() == 59);

    Vector2 dateV = UIDatePicker::preferredSize(1.0f);
    Vector2 dateH = UIDatePicker::preferredSize(1.0f, UIDirection::Horizontal);
    EOKAS_EXPECT(_FloatEqual(dateV.x, 224.0f));
    EOKAS_EXPECT(_FloatEqual(dateV.y, 200.0f));
    EOKAS_EXPECT(_FloatEqual(dateH.x, 200.0f));
    EOKAS_EXPECT(_FloatEqual(dateH.y, 204.0f));
    Vector2 timeV = UITimePicker::preferredSize(1.0f, true);
    Vector2 timeH = UITimePicker::preferredSize(1.0f, true, UIDirection::Horizontal);
    EOKAS_EXPECT(_FloatEqual(timeV.x, 204.0f));
    EOKAS_EXPECT(_FloatEqual(timeV.y, 200.0f));
    EOKAS_EXPECT(_FloatEqual(timeH.x, 200.0f));
    EOKAS_EXPECT(_FloatEqual(timeH.y, 204.0f));
    Vector2 dateTimeV = UIDateTimePicker::preferredSize(1.0f, true);
    Vector2 dateTimeH = UIDateTimePicker::preferredSize(1.0f, true, UIDirection::Horizontal);
    EOKAS_EXPECT(_FloatEqual(dateTimeV.x, 460.0f));
    EOKAS_EXPECT(_FloatEqual(dateTimeV.y, 220.0f));
    EOKAS_EXPECT(_FloatEqual(dateTimeH.x, 220.0f));
    EOKAS_EXPECT(_FloatEqual(dateTimeH.y, 440.0f));

    auto countDirected = [](UIWidget& root, UIDirection expected)
    {
        int count = 0;
        for (const auto& child : root.children())
        {
            auto* numbers = dynamic_cast<UINumberPicker*>(child.get());
            if (numbers != nullptr)
            {
                if (numbers->direction != expected) return -1;
                count += 1;
            }
            auto* date = dynamic_cast<UIDatePicker*>(child.get());
            if (date != nullptr)
            {
                if (date->direction != expected) return -1;
                count += 1;
            }
            auto* time = dynamic_cast<UITimePicker*>(child.get());
            if (time != nullptr)
            {
                if (time->direction != expected) return -1;
                count += 1;
            }
        }
        return count;
    };

    UIDatePicker dateDirected;
    dateDirected.direction = UIDirection::Horizontal;
    dateDirected.shape.setBox(Vector2(0.0f, 0.0f), dateH);
    dateDirected.layout();
    EOKAS_EXPECT(countDirected(dateDirected, UIDirection::Horizontal) == 3);

    UITimePicker timeDirected;
    timeDirected.direction = UIDirection::Horizontal;
    timeDirected.shape.setBox(Vector2(0.0f, 0.0f), timeH);
    timeDirected.layout();
    EOKAS_EXPECT(countDirected(timeDirected, UIDirection::Horizontal) == 3);

    UIDateTimePicker dateTimeDirected;
    dateTimeDirected.direction = UIDirection::Horizontal;
    dateTimeDirected.shape.setBox(Vector2(0.0f, 0.0f), dateTimeH);
    dateTimeDirected.layout();
    EOKAS_EXPECT(countDirected(dateTimeDirected, UIDirection::Horizontal) == 2);

    UIPrimitive primitive;
    primitive.begin();
    primitive.addQuad(Rect(0, 0, 10, 10), Rect(0, 0, 1, 1), Color(1, 1, 1, 1));
    primitive.end();
    EOKAS_EXPECT(primitive.indexCount == 6);
    EOKAS_EXPECT(primitive.vertexLength == 6 * sizeof(UIVertex));

    UIWindow uiWindow;
    uiWindow.init(200, 100);
    auto root = std::make_shared<UIWidget>();
    root->shape.size = (Rect(0, 0, 200, 100)).size;
    root->shape.origin = (Rect(0, 0, 200, 100)).origin + root->shape.pivot * root->shape.size;
    uiWindow.setRoot(root);
    EOKAS_EXPECT(uiWindow.hitTest(10, 10) == root.get());
    uiWindow.onMouseDown(10, 10, UIPointerCode::Left);
    uiWindow.onMouseUp(10, 10, UIPointerCode::Left);
    auto panel = std::make_shared<UIWidget>();
    panel->shape.size = (Rect(50.0f, 60.0f, 80.0f, 40.0f)).size;
    root->placeChild(*panel, Vector2(50.0f, 60.0f));
    auto nested = std::make_shared<UIWidget>();
    nested->shape.size = (Rect(10.0f, 8.0f, 20.0f, 12.0f)).size;
    panel->placeChild(*nested, Vector2(10.0f, 8.0f));
    panel->attachChild(nested);
    root->attachChild(panel);
    EOKAS_EXPECT(uiWindow.hitTest(62.0f, 70.0f) == nested.get());
    EOKAS_EXPECT(uiWindow.hitTest(52.0f, 62.0f) == panel.get());
    uiWindow.quit();

    UIView view;
    auto content = std::make_shared<UIWidget>();
    content->shape.size = (Rect(0, 0, 400, 300)).size;
    content->shape.origin = (Rect(0, 0, 400, 300)).origin + content->shape.pivot * content->shape.size;
    view.addChild(content);
    view.shape.size = (Rect(0, 0, 50, 40)).size;
    view.shape.origin = (Rect(0, 0, 50, 40)).origin + view.shape.pivot * view.shape.size;
    view.setScroll(4, 6);
    EOKAS_EXPECT(_FloatEqual(view.scrollX(), 4.0f));
    EOKAS_EXPECT(_FloatEqual(view.scrollY(), 6.0f));

    UIView scrolledPane;
    scrolledPane.shape.setBox(Vector2(0.0f, 0.0f), Vector2(200.0f, 100.0f));
    auto scrolledBody = std::make_shared<UIWidget>();
    scrolledBody->shape.setBox(Vector2(0.0f, 0.0f), Vector2(260.0f, 140.0f));
    scrolledPane.addChild(scrolledBody);
    UIPrimitive scrolledPrim;
    scrolledPane.layout();
    scrolledPrim.begin();
    scrolledPane.render(scrolledPrim);
    scrolledPane.render(scrolledPrim);
    scrolledPrim.end();
    Rect scrolledBox = scrolledBody->screenBounds();
    EOKAS_EXPECT(_FloatEqual(scrolledBox.origin.x, 0.0f));
    EOKAS_EXPECT(_FloatEqual(scrolledBox.origin.y, 0.0f));
    EOKAS_EXPECT(_FloatEqual(scrolledPane.scrollX(), 0.0f));
    EOKAS_EXPECT(_FloatEqual(scrolledPane.scrollY(), 0.0f));

    UIView paneView;
    paneView.shape.setBox(Vector2(0.0f, 0.0f), Vector2(80.0f, 50.0f));
    auto paneTable = std::make_shared<UITable>();
    paneTable->addColumn(30.0f);
    paneTable->addColumn(40.0f);
    paneTable->addRow(20.0f);
    paneTable->addRow(20.0f);
    paneTable->shape.setBox(Vector2(0.0f, 0.0f), Vector2(70.0f, 40.0f));
    paneView.addChild(paneTable);
    UIPrimitive panePrim;
    paneView.layout();
    panePrim.begin();
    paneView.render(panePrim);
    paneView.render(panePrim);
    panePrim.end();
    Rect paneBox = paneTable->screenBounds();
    EOKAS_EXPECT(_FloatEqual(paneBox.origin.x, 0.0f));
    EOKAS_EXPECT(_FloatEqual(paneBox.origin.y, 0.0f));
    EOKAS_EXPECT(_FloatEqual(paneView.scrollX(), 0.0f));
    EOKAS_EXPECT(_FloatEqual(paneView.scrollY(), 0.0f));

    UIDockSpace dock;
    dock.shape.setBox(Vector2(0, 0), Vector2(400, 300));
    auto page = std::make_shared<UIDockPage>();
    dock.dockPage(page, UIDockMode::Fill);
    UIPrimitive dockPrim;
    dock.layout();
    dockPrim.begin();
    dock.render(dockPrim);
    dockPrim.end();
    EOKAS_EXPECT(page->mode() == UIDockMode::Fill);
    EOKAS_EXPECT(page->space() == &dock);
    Rect bounds;
    EOKAS_EXPECT(dock.pageBounds(page.get(), bounds));
    EOKAS_EXPECT(_FloatEqual(bounds.origin.x, 0.0f));
    EOKAS_EXPECT(_FloatEqual(bounds.origin.y, 0.0f));
    EOKAS_EXPECT(_FloatEqual(bounds.size.x, 400.0f));
    EOKAS_EXPECT(_FloatEqual(bounds.size.y, 300.0f));
    EOKAS_EXPECT(dock.showPreview(200.0f, 150.0f));
    EOKAS_EXPECT(dock.previewMode() == UIDockMode::Fill);
    EOKAS_EXPECT(dock.showPreview(8.0f, 150.0f));
    EOKAS_EXPECT(dock.previewMode() == UIDockMode::Left);
    EOKAS_EXPECT(dock.showPreview(392.0f, 150.0f));
    EOKAS_EXPECT(dock.previewMode() == UIDockMode::Right);
    EOKAS_EXPECT(dock.showPreview(119.0f, 150.0f));
    EOKAS_EXPECT(dock.previewMode() == UIDockMode::Left);
    EOKAS_EXPECT(dock.showPreview(121.0f, 150.0f));
    EOKAS_EXPECT(dock.previewMode() == UIDockMode::Fill);
    EOKAS_EXPECT(dock.showPreview(280.0f, 150.0f));
    EOKAS_EXPECT(dock.previewMode() == UIDockMode::Right);
    EOKAS_EXPECT(dock.showPreview(200.0f, 40.0f));
    EOKAS_EXPECT(dock.previewMode() == UIDockMode::Top);
    EOKAS_EXPECT(dock.showPreview(200.0f, 250.0f));
    EOKAS_EXPECT(dock.previewMode() == UIDockMode::Bottom);

    int windows = 0;
    UIHost app;
    app.onCreateWindow = [&](UIWindow&, const Rect&) {
        windows += 1;
    };
    UIWindow& opened = app.open(320, 240);
    app.layout(opened, 320, 240);
    EOKAS_EXPECT(opened.root() != nullptr);
    EOKAS_EXPECT(_FloatEqual(opened.root()->shape.size.x, 320.0f));
    EOKAS_EXPECT(_FloatEqual(opened.root()->shape.size.y, 240.0f));
    app.close(opened);
    EOKAS_EXPECT(windows == 0);

    UIWidget scaled;
    EOKAS_EXPECT(_FloatEqual(scaled.shape.scale.x, 1.0f));
    EOKAS_EXPECT(_FloatEqual(scaled.shape.scale.y, 1.0f));
    scaled.shape.scale = Vector2(2.0f, 3.0f);
    scaled.shape.setBox(Vector2(1.0f, 2.0f), Vector2(3.0f, 4.0f));
    EOKAS_EXPECT(_FloatEqual(scaled.shape.scale.x, 2.0f));
    EOKAS_EXPECT(_FloatEqual(scaled.shape.scale.y, 3.0f));
    EOKAS_EXPECT(_FloatEqual(scaled.shape.origin.x, 2.5f));
    EOKAS_EXPECT(_FloatEqual(scaled.shape.origin.y, 4.0f));
    EOKAS_EXPECT(_FloatEqual(scaled.shape.bounds(scaled.shape.localTrans()).origin.x, -0.5f));
    EOKAS_EXPECT(_FloatEqual(scaled.shape.bounds(scaled.shape.localTrans()).origin.y, -2.0f));
    EOKAS_EXPECT(_FloatEqual(scaled.shape.bounds(scaled.shape.localTrans()).size.x, 6.0f));
    EOKAS_EXPECT(_FloatEqual(scaled.shape.bounds(scaled.shape.localTrans()).size.y, 12.0f));

    UIWindow scaleWindow;
    scaleWindow.init(200, 100);
    auto scaleRoot = std::make_shared<UIWidget>();
    scaleRoot->shape.size = (Rect(0, 0, 200, 100)).size;
    scaleRoot->shape.origin = (Rect(0, 0, 200, 100)).origin + scaleRoot->shape.pivot * scaleRoot->shape.size;
    auto scaledBox = std::make_shared<UIWidget>();
    scaledBox->shape.size = (Rect(10.0f, 10.0f, 20.0f, 20.0f)).size;
    scaleRoot->placeChild(*scaledBox, Vector2(10.0f, 10.0f));
    scaledBox->shape.scale = Vector2(2.0f, 2.0f);
    scaleRoot->attachChild(scaledBox);
    scaleWindow.setRoot(scaleRoot);
    EOKAS_EXPECT(scaleWindow.hitTest(0.0f, 0.0f) == scaledBox.get());
    EOKAS_EXPECT(scaleWindow.hitTest(40.0f, 40.0f) == scaledBox.get());
    EOKAS_EXPECT(scaleWindow.hitTest(41.0f, 41.0f) == scaleRoot.get());

    auto scaledParent = std::make_shared<UIWidget>();
    scaledParent->shape.size = (Rect(0.0f, 0.0f, 100.0f, 40.0f)).size;
    scaleRoot->placeChild(*scaledParent, Vector2(0.0f, 0.0f));
    scaledParent->shape.scale = Vector2(2.0f, 1.0f);
    auto nestedScale = std::make_shared<UIWidget>();
    nestedScale->shape.size = (Rect(10.0f, 0.0f, 10.0f, 10.0f)).size;
    scaledParent->placeChild(*nestedScale, Vector2(10.0f, 0.0f));
    scaledParent->attachChild(nestedScale);
    scaleRoot->detachChildren();
    scaleRoot->attachChild(scaledParent);
    EOKAS_EXPECT(scaleWindow.hitTest(-30.0f, 0.0f) == nestedScale.get());
    EOKAS_EXPECT(scaleWindow.hitTest(-10.0f, 0.0f) == nestedScale.get());
    EOKAS_EXPECT(scaleWindow.hitTest(-31.0f, 0.0f) == scaledParent.get());
    EOKAS_EXPECT(scaleWindow.hitTest(20.0f, 0.0f) == scaledParent.get());
    scaleWindow.quit();

    UICanvas fitted;
    fitted.shape.size = (Rect(100.0f, 50.0f, 0.0f, 0.0f)).size;
    fitted.shape.origin = (Rect(100.0f, 50.0f, 0.0f, 0.0f)).origin + fitted.shape.pivot * fitted.shape.size;
    auto top = std::make_shared<UIWidget>();
    auto low = std::make_shared<UIWidget>();
    top->shape.size = (Rect(0.0f, 0.0f, 10.0f, 20.0f)).size;
    top->shape.origin = (Rect(0.0f, 0.0f, 10.0f, 20.0f)).origin + top->shape.pivot * top->shape.size;
    low->shape.size = (Rect(30.0f, -5.0f, 10.0f, 10.0f)).size;
    low->shape.origin = (Rect(30.0f, -5.0f, 10.0f, 10.0f)).origin + low->shape.pivot * low->shape.size;
    fitted.attachChild(top);
    fitted.attachChild(low);
    fitted.refit();
    EOKAS_EXPECT(_FloatEqual(boxLeft(fitted.shape), 100.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(fitted.shape), 45.0f));
    EOKAS_EXPECT(_FloatEqual(fitted.shape.size.x, 40.0f));
    EOKAS_EXPECT(_FloatEqual(fitted.shape.size.y, 25.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(top->shape), -7.5f));
    EOKAS_EXPECT(_FloatEqual(boxTop(low->shape), -12.5f));
    top->shape.origin.x = pivotSpace(fitted.shape, Vector2(-4.0f, 0.0f)).x + top->shape.pivot.x * top->shape.size.x;
    fitted.refit();
    EOKAS_EXPECT(_FloatEqual(boxLeft(fitted.shape), 96.0f));
    EOKAS_EXPECT(_FloatEqual(fitted.shape.size.x, 44.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(top->shape), -22.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(low->shape), 12.0f));

    UICanvas scaledCanvas;
    scaledCanvas.shape.scale = Vector2(2.0f, 2.0f);
    scaledCanvas.shape.size = (Rect(100.0f, 50.0f, 0.0f, 0.0f)).size;
    scaledCanvas.shape.origin = (Rect(100.0f, 50.0f, 0.0f, 0.0f)).origin + scaledCanvas.shape.pivot * scaledCanvas.shape.size;
    auto leftItem = std::make_shared<UIWidget>();
    auto rightItem = std::make_shared<UIWidget>();
    leftItem->shape.size = (Rect(0.0f, 0.0f, 10.0f, 20.0f)).size;
    leftItem->shape.origin = (Rect(0.0f, 0.0f, 10.0f, 20.0f)).origin + leftItem->shape.pivot * leftItem->shape.size;
    rightItem->shape.size = (Rect(30.0f, -5.0f, 10.0f, 10.0f)).size;
    rightItem->shape.origin = (Rect(30.0f, -5.0f, 10.0f, 10.0f)).origin + rightItem->shape.pivot * rightItem->shape.size;
    scaledCanvas.attachChild(leftItem);
    scaledCanvas.attachChild(rightItem);
    scaledCanvas.refit();
    leftItem->shape.origin.x = pivotSpace(scaledCanvas.shape, Vector2(-4.0f, 0.0f)).x + leftItem->shape.pivot.x * leftItem->shape.size.x;
    scaledCanvas.refit();
    EOKAS_EXPECT(_FloatEqual(boxLeft(scaledCanvas.shape), 114.0f));
    EOKAS_EXPECT(_FloatEqual(scaledCanvas.shape.size.x, 44.0f));
    EOKAS_EXPECT(_FloatEqual(scaledCanvas.shape.bounds(scaledCanvas.shape.localTrans()).size.x, 88.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(leftItem->shape), -22.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(rightItem->shape), 12.0f));

    UIWindow canvasWindow;
    canvasWindow.init(400, 300);
    auto canvas = std::make_shared<UICanvas>();
    canvas->shape.size = (Rect(100.0f, 50.0f, 0.0f, 0.0f)).size;
    canvas->shape.origin = (Rect(100.0f, 50.0f, 0.0f, 0.0f)).origin + canvas->shape.pivot * canvas->shape.size;
    auto held = std::make_shared<UIWidget>();
    auto loose = std::make_shared<UIWidget>();
    held->shape.size = (Rect(0.0f, 0.0f, 10.0f, 20.0f)).size;
    held->shape.origin = (Rect(0.0f, 0.0f, 10.0f, 20.0f)).origin + held->shape.pivot * held->shape.size;
    loose->shape.size = (Rect(30.0f, -5.0f, 10.0f, 10.0f)).size;
    loose->shape.origin = (Rect(30.0f, -5.0f, 10.0f, 10.0f)).origin + loose->shape.pivot * loose->shape.size;
    loose->draggable = false;
    canvas->attachChild(held);
    canvas->attachChild(loose);
    canvasWindow.setRoot(canvas);
    canvasWindow.tick(0.0f);
    canvasWindow.hitTest(0.0f, 0.0f);
    Rect heldBefore = Rect(boxLeft(held->shape), boxTop(held->shape), held->shape.size.x, held->shape.size.y);
    Rect looseBefore = Rect(boxLeft(loose->shape), boxTop(loose->shape), loose->shape.size.x, loose->shape.size.y);
    canvasWindow.onMouseDown(120.0f, 65.0f, UIPointerCode::Left);
    canvasWindow.onMouseMove(130.0f, 70.0f);
    canvasWindow.onMouseUp(130.0f, 70.0f, UIPointerCode::Left);
    EOKAS_EXPECT(_FloatEqual(boxLeft(held->shape), heldBefore.origin.x));
    EOKAS_EXPECT(_FloatEqual(boxTop(held->shape), heldBefore.origin.y));
    EOKAS_EXPECT(_FloatEqual(boxLeft(loose->shape), looseBefore.origin.x));
    EOKAS_EXPECT(_FloatEqual(boxTop(loose->shape), looseBefore.origin.y));
    EOKAS_EXPECT(_FloatEqual(boxLeft(canvas->shape), 110.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(canvas->shape), 50.0f));

    canvasWindow.onMouseDown(140.0f, 50.0f, UIPointerCode::Left);
    canvasWindow.onMouseMove(150.0f, 50.0f);
    canvasWindow.onMouseUp(150.0f, 50.0f, UIPointerCode::Left);
    EOKAS_EXPECT(_FloatEqual(boxLeft(loose->shape), looseBefore.origin.x));
    loose->draggable = true;
    canvasWindow.onMouseDown(140.0f, 50.0f, UIPointerCode::Left);
    canvasWindow.onMouseMove(150.0f, 50.0f);
    canvasWindow.onMouseUp(150.0f, 50.0f, UIPointerCode::Left);
    EOKAS_EXPECT(_FloatEqual(boxLeft(loose->shape), looseBefore.origin.x + 5.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(held->shape), heldBefore.origin.x - 5.0f));

    Vector2 focal = Vector2(boxLeft(canvas->shape), boxTop(canvas->shape)) + Vector2(10.0f, 10.0f);
    Vector2 focalLocal = canvas->shape.pivotToLocal(focal);
    Rect looseAtScale = Rect(boxLeft(loose->shape), boxTop(loose->shape), loose->shape.size.x, loose->shape.size.y);
    canvasWindow.onMouseWheel(focal.x, focal.y, 0.0f, -48.0f);
    Vector2 back = UIShape::transformPoint(canvas->shape.localTrans(), focalLocal);
    EOKAS_EXPECT(_FloatEqual(back.x, focal.x));
    EOKAS_EXPECT(_FloatEqual(back.y, focal.y));
    EOKAS_EXPECT(_FloatEqual(boxLeft(loose->shape), looseAtScale.origin.x));
    EOKAS_EXPECT(_FloatEqual(boxTop(loose->shape), looseAtScale.origin.y));
    Vector2 corner(boxLeft(canvas->shape), boxTop(canvas->shape));
    Vector2 cornerLocal = canvas->shape.pivotToLocal(corner);
    canvas->scaleAt(corner, canvas->shape.scale.x * 2.0f);
    Vector2 cornerBack = UIShape::transformPoint(canvas->shape.localTrans(), cornerLocal);
    EOKAS_EXPECT(std::fabs(cornerBack.x - corner.x) <= 0.0001f);
    EOKAS_EXPECT(std::fabs(cornerBack.y - corner.y) <= 0.0001f);

    auto dragButton = std::make_shared<UIButton>();
    dragButton->setText("OK");
    dragButton->shape.size = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).size;
    dragButton->shape.origin = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).origin + dragButton->shape.pivot * dragButton->shape.size;
    dragButton->draggable = true;
    UICanvas dragGroup;
    dragGroup.attachChild(dragButton);
    dragGroup.shape.setBox(Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f));
    UIPrimitive dragPrim;
    dragGroup.layout();
    dragPrim.begin();
    dragGroup.render(dragPrim);
    dragPrim.end();
    Rect labelRect = Rect(boxLeft(dragButton->label()->shape), boxTop(dragButton->label()->shape), dragButton->label()->shape.size.x, dragButton->label()->shape.size.y);
    dragGroup.dragChild(dragButton.get(), 5.0f, 6.0f);
    dragGroup.dragChild(dragButton.get(), 15.0f, 9.0f);
    EOKAS_EXPECT(_FloatEqual(boxLeft(dragGroup.shape), 10.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(dragGroup.shape), 3.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(dragButton->shape), -20.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(dragButton->shape), -10.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(dragButton->label()->shape), labelRect.origin.x));
    EOKAS_EXPECT(_FloatEqual(boxTop(dragButton->label()->shape), labelRect.origin.y));

    UIChart triangle;
    triangle.setContour({ Vector2(0.0f, 0.0f), Vector2(40.0f, 0.0f), Vector2(0.0f, 40.0f) });
    EOKAS_EXPECT(triangle.contains(Vector2(5.0f, 5.0f)));
    EOKAS_EXPECT(!triangle.contains(Vector2(30.0f, 30.0f)));

    UIWindow shapeWindow;
    shapeWindow.init(200, 200);
    auto shapeRoot = std::make_shared<UIWidget>();
    shapeRoot->shape.size = (Rect(0.0f, 0.0f, 200.0f, 200.0f)).size;
    shapeRoot->shape.origin = (Rect(0.0f, 0.0f, 200.0f, 200.0f)).origin + shapeRoot->shape.pivot * shapeRoot->shape.size;
    shapeRoot->pickable = false;
    auto lowerChart = std::make_shared<UIChart>();
    auto upperChart = std::make_shared<UIChart>();
    auto atRoot = [&](float x, float y) { return pivotSpace(shapeRoot->shape, Vector2(x, y)); };
    lowerChart->setContour({ atRoot(0.0f, 0.0f), atRoot(40.0f, 0.0f), atRoot(40.0f, 40.0f), atRoot(0.0f, 40.0f) });
    upperChart->setContour({ atRoot(0.0f, 0.0f), atRoot(40.0f, 0.0f), atRoot(0.0f, 40.0f) });
    shapeRoot->attachChild(lowerChart);
    shapeRoot->attachChild(upperChart);
    shapeWindow.setRoot(shapeRoot);
    EOKAS_EXPECT(shapeWindow.hitTest(5.0f, 5.0f) == upperChart.get());
    EOKAS_EXPECT(shapeWindow.hitTest(30.0f, 5.0f) == upperChart.get());
    EOKAS_EXPECT(shapeWindow.hitTest(30.0f, 30.0f) == lowerChart.get());

    auto parentChart = std::make_shared<UIChart>();
    parentChart->setContour({ atRoot(0.0f, 0.0f), atRoot(80.0f, 0.0f), atRoot(0.0f, 80.0f) });
    auto atParent = [&](float x, float y) { return pivotSpace(parentChart->shape, Vector2(x, y)); };
    auto innerChart = std::make_shared<UIChart>();
    innerChart->setContour({ atParent(4.0f, 4.0f), atParent(16.0f, 4.0f), atParent(4.0f, 16.0f) });
    auto outerChart = std::make_shared<UIChart>();
    outerChart->setContour({ atParent(50.0f, 50.0f), atParent(70.0f, 50.0f), atParent(50.0f, 70.0f) });
    parentChart->addChart(innerChart);
    parentChart->addChart(outerChart);
    shapeRoot->detachChildren();
    shapeRoot->attachChild(parentChart);
    EOKAS_EXPECT(shapeWindow.hitTest(6.0f, 6.0f) == innerChart.get());
    EOKAS_EXPECT(shapeWindow.hitTest(2.0f, 2.0f) == parentChart.get());
    EOKAS_EXPECT(shapeWindow.hitTest(55.0f, 55.0f) == nullptr);
    parentChart->pickable = false;
    EOKAS_EXPECT(shapeWindow.hitTest(2.0f, 2.0f) == nullptr);
    shapeWindow.quit();

    UIWindow chartWindow;
    chartWindow.init(300, 200);
    auto chartCanvas = std::make_shared<UICanvas>();
    chartCanvas->shape.size = (Rect(0.0f, 0.0f, 300.0f, 200.0f)).size;
    chartCanvas->shape.origin = (Rect(0.0f, 0.0f, 300.0f, 200.0f)).origin + chartCanvas->shape.pivot * chartCanvas->shape.size;
    auto atChart = [&](float x, float y) { return pivotSpace(chartCanvas->shape, Vector2(x, y)); };
    auto anchor = std::make_shared<UIWidget>();
    anchor->shape.size = (Rect(0.0f, 0.0f, 8.0f, 8.0f)).size;
    anchor->shape.origin = atChart(0.0f, 0.0f) + anchor->shape.pivot * anchor->shape.size;
    anchor->pickable = false;
    auto picked = std::make_shared<UIChart>();
    auto idle = std::make_shared<UIChart>();
    picked->setContour({ atChart(20.0f, 20.0f), atChart(60.0f, 20.0f), atChart(20.0f, 60.0f) });
    idle->setContour({ atChart(70.0f, 20.0f), atChart(110.0f, 20.0f), atChart(70.0f, 60.0f) });
    chartCanvas->attachChild(anchor);
    chartCanvas->attachChild(picked);
    chartCanvas->attachChild(idle);
    chartWindow.setRoot(chartCanvas);
    int chartClicks = 0;
    int chartDoubles = 0;
    picked->onClick = [&]() { chartClicks += 1; };
    picked->onDoubleClick = [&]() { chartDoubles += 1; };
    chartWindow.onMouseDown(30.0f, 30.0f, UIPointerCode::Left);
    chartWindow.onMouseUp(30.0f, 30.0f, UIPointerCode::Left);
    EOKAS_EXPECT(chartClicks == 1);
    EOKAS_EXPECT(picked->selected);
    EOKAS_EXPECT(!idle->selected);
    chartWindow.onMouseDown(30.0f, 30.0f, UIPointerCode::Left);
    chartWindow.onMouseUp(30.0f, 30.0f, UIPointerCode::Left);
    EOKAS_EXPECT(chartDoubles == 1);

    std::vector<Vector2> contourBefore = picked->contour();
    picked->selected = true;
    idle->selected = false;
    chartWindow.onMouseDown(30.0f, 30.0f, UIPointerCode::Left);
    chartWindow.onMouseMove(42.0f, 30.0f);
    chartWindow.onMouseUp(42.0f, 30.0f, UIPointerCode::Left);
    EOKAS_EXPECT(chartClicks == 1);
    EOKAS_EXPECT(picked->selected);
    EOKAS_EXPECT(!idle->selected);
    EOKAS_EXPECT(_FloatEqual(boxLeft(picked->shape), contourBefore.empty() ? 0.0f : -23.0f));
    EOKAS_EXPECT(picked->contour().size() == contourBefore.size());
    EOKAS_EXPECT(_FloatEqual(picked->contour()[0].x, contourBefore[0].x));
    EOKAS_EXPECT(_FloatEqual(picked->contour()[0].y, contourBefore[0].y));
    chartWindow.quit();

    UIWindow dropWindow;
    dropWindow.init(300, 120);
    auto dropCanvas = std::make_shared<UICanvas>();
    dropCanvas->shape.size = (Rect(0.0f, 0.0f, 300.0f, 120.0f)).size;
    dropCanvas->shape.origin = (Rect(0.0f, 0.0f, 300.0f, 120.0f)).origin + dropCanvas->shape.pivot * dropCanvas->shape.size;
    auto atDrop = [&](float x, float y) { return pivotSpace(dropCanvas->shape, Vector2(x, y)); };
    auto carried = std::make_shared<UIChart>();
    auto slot = std::make_shared<UIChart>();
    carried->setContour({ atDrop(0.0f, 0.0f), atDrop(40.0f, 0.0f), atDrop(0.0f, 40.0f) });
    slot->setContour({ atDrop(80.0f, 0.0f), atDrop(120.0f, 0.0f), atDrop(80.0f, 40.0f) });
    dropCanvas->attachChild(carried);
    dropCanvas->attachChild(slot);
    dropWindow.setRoot(dropCanvas);
    int drops = 0;
    UIWidget* dropTarget = nullptr;
    carried->onChartDrop = [&](UIWidget* target, float, float) {
        drops += 1;
        dropTarget = target;
    };
    dropWindow.onMouseDown(10.0f, 10.0f, UIPointerCode::Left);
    dropWindow.onMouseMove(90.0f, 10.0f);
    dropWindow.onMouseUp(90.0f, 10.0f, UIPointerCode::Left);
    EOKAS_EXPECT(drops == 1);
    EOKAS_EXPECT(dropTarget == slot.get());

    auto plainCanvas = std::make_shared<UICanvas>();
    plainCanvas->shape.size = (Rect(0.0f, 0.0f, 300.0f, 120.0f)).size;
    plainCanvas->shape.origin = (Rect(0.0f, 0.0f, 300.0f, 120.0f)).origin + plainCanvas->shape.pivot * plainCanvas->shape.size;
    auto atPlain = [&](float x, float y) { return pivotSpace(plainCanvas->shape, Vector2(x, y)); };
    auto stone = std::make_shared<UIChart>();
    stone->setContour({ atPlain(60.0f, 0.0f), atPlain(100.0f, 0.0f), atPlain(60.0f, 40.0f) });
    auto plain = std::make_shared<UIWidget>();
    plain->shape.size = (Rect(0.0f, 0.0f, 40.0f, 40.0f)).size;
    plain->shape.origin = atPlain(0.0f, 0.0f) + plain->shape.pivot * plain->shape.size;
    plainCanvas->attachChild(stone);
    plainCanvas->attachChild(plain);
    dropWindow.setRoot(plainCanvas);
    int plainDrops = 0;
    UIWidget* plainTarget = stone.get();
    stone->onChartDrop = [&](UIWidget* target, float, float) {
        plainDrops += 1;
        plainTarget = target;
    };
    dropWindow.onMouseDown(70.0f, 10.0f, UIPointerCode::Left);
    dropWindow.onMouseMove(10.0f, 10.0f);
    dropWindow.onMouseUp(10.0f, 10.0f, UIPointerCode::Left);
    EOKAS_EXPECT(plainDrops == 1);
    EOKAS_EXPECT(plainTarget == nullptr);
    dropWindow.quit();

    UIWindow zoomWindow;
    zoomWindow.init(200, 200);
    auto zoomCanvas = std::make_shared<UICanvas>();
    zoomCanvas->shape.size = (Rect(0.0f, 0.0f, 200.0f, 200.0f)).size;
    zoomCanvas->shape.origin = (Rect(0.0f, 0.0f, 200.0f, 200.0f)).origin + zoomCanvas->shape.pivot * zoomCanvas->shape.size;
    auto zoomChart = std::make_shared<UIChart>();
    zoomChart->setContour({
        pivotSpace(zoomCanvas->shape, Vector2(0.0f, 0.0f)),
        pivotSpace(zoomCanvas->shape, Vector2(40.0f, 0.0f)),
        pivotSpace(zoomCanvas->shape, Vector2(0.0f, 40.0f))
    });
    zoomCanvas->attachChild(zoomChart);
    zoomWindow.setRoot(zoomCanvas);
    Vector2 zoomFocal = Vector2(10.0f, 10.0f) - zoomCanvas->shape.origin;
    Vector2 chartLocal = zoomChart->shape.pivotToLocal(zoomFocal);
    zoomWindow.onMouseWheel(10.0f, 10.0f, 0.0f, -48.0f);
    EOKAS_EXPECT(zoomChart->shape.scale.x > 1.0f);
    EOKAS_EXPECT(_FloatEqual(zoomCanvas->shape.scale.x, 1.0f));
    Vector2 zoomBack = UIShape::transformPoint(zoomChart->shape.localTrans(), chartLocal);
    EOKAS_EXPECT(_FloatEqual(zoomBack.x, zoomFocal.x));
    EOKAS_EXPECT(_FloatEqual(zoomBack.y, zoomFocal.y));
    Vector2 canvasFocal(180.0f, 180.0f);
    zoomWindow.onMouseWheel(canvasFocal.x, canvasFocal.y, 0.0f, -48.0f);
    EOKAS_EXPECT(zoomCanvas->shape.scale.x > 1.0f);
    EOKAS_EXPECT(_FloatEqual(zoomChart->shape.bounds(zoomChart->shape.localTrans()).origin.x, -zoomCanvas->shape.pivot.x * zoomCanvas->shape.size.x));
    EOKAS_EXPECT(_FloatEqual(zoomChart->shape.bounds(zoomChart->shape.localTrans()).origin.y, -zoomCanvas->shape.pivot.y * zoomCanvas->shape.size.y));
    zoomWindow.quit();

    UIEllipse ellipse;
    ellipse.shape.size = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).size;
    ellipse.shape.origin = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).origin + ellipse.shape.pivot * ellipse.shape.size;
    EOKAS_EXPECT(ellipse.contains(Vector2(20.0f, 10.0f)));
    EOKAS_EXPECT(ellipse.contains(Vector2(40.0f, 10.0f)));
    EOKAS_EXPECT(!ellipse.contains(Vector2(0.0f, 0.0f)));
    EOKAS_EXPECT(_FloatEqual(ellipse.shape.size.x, 40.0f));
    EOKAS_EXPECT(_FloatEqual(ellipse.shape.size.y, 20.0f));
    UIPrimitive ellipsePrimitive;
    ellipsePrimitive.begin();
    ellipse.render(ellipsePrimitive);
    ellipsePrimitive.end();
    uint32_t fillCount = ellipsePrimitive.indexCount;
    ellipse.stroke.thickness = 2.0f;
    ellipse.selected = true;
    ellipsePrimitive.begin();
    ellipse.render(ellipsePrimitive);
    ellipsePrimitive.end();
    EOKAS_EXPECT(ellipsePrimitive.indexCount > fillCount);

    UIRectangle chartBox;
    chartBox.shape.size = (Rect(10.0f, 20.0f, 40.0f, 20.0f)).size;
    chartBox.shape.origin = (Rect(10.0f, 20.0f, 40.0f, 20.0f)).origin + chartBox.shape.pivot * chartBox.shape.size;
    EOKAS_EXPECT(chartBox.contains(chartBox.shape.pivotToLocal(Vector2(10.0f, 20.0f))));
    EOKAS_EXPECT(chartBox.contains(chartBox.shape.pivotToLocal(Vector2(30.0f, 30.0f))));
    EOKAS_EXPECT(chartBox.contains(chartBox.shape.pivotToLocal(Vector2(50.0f, 40.0f))));
    EOKAS_EXPECT(!chartBox.contains(chartBox.shape.pivotToLocal(Vector2(9.0f, 30.0f))));

    UIDiamond diamond;
    diamond.shape.size = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).size;
    diamond.shape.origin = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).origin + diamond.shape.pivot * diamond.shape.size;
    EOKAS_EXPECT(diamond.contains(Vector2(20.0f, 10.0f)));
    EOKAS_EXPECT(diamond.contains(Vector2(20.0f, 0.0f)));
    EOKAS_EXPECT(diamond.contains(Vector2(40.0f, 10.0f)));
    EOKAS_EXPECT(!diamond.contains(Vector2(0.0f, 0.0f)));
    EOKAS_EXPECT(!diamond.contains(Vector2(40.0f, 0.0f)));

    auto from = std::make_shared<UIRectangle>();
    auto to = std::make_shared<UIRectangle>();
    from->shape.size = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).size;
    from->shape.origin = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).origin + from->shape.pivot * from->shape.size;
    to->shape.size = (Rect(80.0f, 0.0f, 40.0f, 20.0f)).size;
    to->shape.origin = (Rect(80.0f, 0.0f, 40.0f, 20.0f)).origin + to->shape.pivot * to->shape.size;
    UILink link;
    link.kind = UIPathKind::Straight;
    link.setPoints({ Vector2(40.0f, 10.0f), Vector2(80.0f, 10.0f) });
    link.line.thickness = 2.0f;
    link.hitSlop = 4.0f;
    EOKAS_EXPECT(link.contains(link.shape.pivotToLocal(Vector2(60.0f, 10.0f))));
    EOKAS_EXPECT(!link.contains(link.shape.pivotToLocal(Vector2(60.0f, 30.0f))));
    UIWindow linkWindow;
    linkWindow.init(200, 80);
    auto linkRoot = std::make_shared<UIWidget>();
    linkRoot->shape.size = (Rect(0.0f, 0.0f, 200.0f, 80.0f)).size;
    linkRoot->shape.origin = (Rect(0.0f, 0.0f, 200.0f, 80.0f)).origin + linkRoot->shape.pivot * linkRoot->shape.size;
    linkRoot->pickable = false;
    auto hitLink = std::make_shared<UILink>();
    hitLink->setPoints({ pivotSpace(linkRoot->shape, Vector2(40.0f, 10.0f)), pivotSpace(linkRoot->shape, Vector2(80.0f, 10.0f)) });
    hitLink->line.thickness = 2.0f;
    hitLink->hitSlop = 4.0f;
    linkRoot->attachChild(hitLink);
    linkWindow.setRoot(linkRoot);
    Vector2 linkPoint = pivotSpace(linkRoot->shape, Vector2(60.0f, 12.0f));
    EOKAS_EXPECT(hitLink->contains(hitLink->shape.pivotToLocal(linkPoint)));
    EOKAS_EXPECT(!Rect(boxLeft(hitLink->shape), boxTop(hitLink->shape), hitLink->shape.size.x, hitLink->shape.size.y).contains(linkPoint));
    EOKAS_EXPECT(linkWindow.hitTest(60.0f, 12.0f) == hitLink.get());
    linkWindow.quit();
    link.start.target = from.get();
    link.start.anchor = UIAnchor::Right;
    link.end.target = to.get();
    link.end.anchor = UIAnchor::Left;
    EOKAS_EXPECT(link.contains(link.shape.pivotToLocal(Vector2(60.0f, 10.0f))));
    to->shape.origin = (Vector2(80.0f, 40.0f)) + to->shape.pivot * to->shape.size;
    EOKAS_EXPECT(link.contains(link.shape.pivotToLocal(Vector2(60.0f, 30.0f))));
    EOKAS_EXPECT(!link.contains(link.shape.pivotToLocal(Vector2(60.0f, 10.0f))));

    from->shape.size = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).size;
    from->shape.origin = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).origin + from->shape.pivot * from->shape.size;
    to->shape.size = (Rect(80.0f, 40.0f, 40.0f, 20.0f)).size;
    to->shape.origin = (Rect(80.0f, 40.0f, 40.0f, 20.0f)).origin + to->shape.pivot * to->shape.size;
    UILink curve;
    curve.kind = UIPathKind::CubicBezier;
    curve.setPoints({
        Vector2(40.0f, 10.0f),
        Vector2(50.0f, -40.0f),
        Vector2(70.0f, 80.0f),
        Vector2(80.0f, 10.0f)
    });
    curve.line.thickness = 2.0f;
    curve.hitSlop = 3.0f;
    curve.start.target = from.get();
    curve.start.anchor = UIAnchor::Right;
    curve.end.target = to.get();
    curve.end.anchor = UIAnchor::Left;
    EOKAS_EXPECT(curve.contains(curve.shape.pivotToLocal(Vector2(44.0f, 10.0f))));
    EOKAS_EXPECT(curve.contains(curve.shape.pivotToLocal(Vector2(76.0f, 50.0f))));
    EOKAS_EXPECT(!curve.contains(curve.shape.pivotToLocal(Vector2(48.0f, -20.0f))));

    UIRectangle cornerBox;
    cornerBox.shape.size = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).size;
    cornerBox.shape.origin = (Rect(0.0f, 0.0f, 40.0f, 20.0f)).origin + cornerBox.shape.pivot * cornerBox.shape.size;
    cornerBox.border.thickness = 2.0f;
    UIPrimitive cornerPrimitive;
    cornerPrimitive.begin();
    cornerBox.render(cornerPrimitive);
    cornerPrimitive.end();
    EOKAS_EXPECT(cornerPrimitive.indexCount == 54);

    canvasWindow.quit();

    UIShape defaults;
    EOKAS_EXPECT(_FloatEqual(defaults.pivot.x, 0.5f));
    EOKAS_EXPECT(_FloatEqual(defaults.pivot.y, 0.5f));
    EOKAS_EXPECT(_FloatEqual(defaults.scale.x, 1.0f));
    EOKAS_EXPECT(_FloatEqual(defaults.scale.y, 1.0f));
    EOKAS_EXPECT(_FloatEqual(defaults.origin.x, 0.0f));
    EOKAS_EXPECT(_FloatEqual(defaults.origin.y, 0.0f));

    UIShape placed;
    placed.setBox(Vector2(1.0f, 2.0f), Vector2(10.0f, 8.0f));
    EOKAS_EXPECT(_FloatEqual(boxLeft(placed), 1.0f));
    EOKAS_EXPECT(_FloatEqual(boxTop(placed), 2.0f));
    EOKAS_EXPECT(_FloatEqual(placed.origin.x, 6.0f));
    EOKAS_EXPECT(_FloatEqual(placed.origin.y, 6.0f));

    UIShape scaledShape;
    scaledShape.scale = Vector2(2.0f, 3.0f);
    scaledShape.setBox(Vector2(1.0f, 2.0f), Vector2(3.0f, 4.0f));
    Rect scaledVisual = scaledShape.bounds(scaledShape.localTrans());
    EOKAS_EXPECT(_FloatEqual(scaledVisual.origin.x, -0.5f));
    EOKAS_EXPECT(_FloatEqual(scaledVisual.origin.y, -2.0f));
    EOKAS_EXPECT(_FloatEqual(scaledVisual.size.x, 6.0f));
    EOKAS_EXPECT(_FloatEqual(scaledVisual.size.y, 12.0f));

    UIShape zoomed;
    zoomed.setBox(Vector2(0.0f, 0.0f), Vector2(10.0f, 10.0f));
    zoomed.scale = Vector2(2.0f, 2.0f);
    Vector2 before = zoomed.pivotToLocal(Vector2(5.0f, 5.0f));
    zoomed.setScaleAround(Vector2(5.0f, 5.0f), Vector2(4.0f, 4.0f));
    Vector2 after = zoomed.pivotToLocal(Vector2(5.0f, 5.0f));
    EOKAS_EXPECT(_FloatEqual(before.x, after.x));
    EOKAS_EXPECT(_FloatEqual(before.y, after.y));
    EOKAS_EXPECT(_FloatEqual(zoomed.origin.x, 5.0f));
    EOKAS_EXPECT(_FloatEqual(zoomed.origin.y, 5.0f));

    UIShape cornerPivot;
    cornerPivot.pivot = Vector2(0.0f, 0.0f);
    cornerPivot.origin = Vector2(0.0f, 0.0f);
    cornerPivot.setScaleAround(Vector2(10.0f, 10.0f), Vector2(2.0f, 2.0f));
    EOKAS_EXPECT(_FloatEqual(cornerPivot.origin.x, -10.0f));
    EOKAS_EXPECT(_FloatEqual(cornerPivot.origin.y, -10.0f));

    UIPrimitive heavy;
    heavy.begin();
    Rect solid = UIFont::solidUV();
    for (int i = 0; i < 3000; ++i)
    {
        heavy.addQuad(Rect((float)i, 0.0f, 1.0f, 1.0f), solid, Color(1.0f, 1.0f, 1.0f, 1.0f));
    }
    heavy.end();
    EOKAS_EXPECT(heavy.indexCount == 18000u);


    UIShape turned;
    turned.angle = 1.5707963f;
    turned.setBox(Vector2(0.0f, 0.0f), Vector2(10.0f, 0.0f));
    Vector2 tip = UIShape::transformPoint(turned.localTrans(), Vector2(10.0f, 0.0f));
    EOKAS_EXPECT(_FloatEqual(tip.x, 5.0f));
    EOKAS_EXPECT(_FloatEqual(tip.y, 5.0f));

    auto parentWidget = std::make_shared<UIWidget>();
    parentWidget->shape.setBox(Vector2(0.0f, 0.0f), Vector2(20.0f, 20.0f));
    auto childWidget = std::make_shared<UIWidget>();
    childWidget->shape.size = Vector2(4.0f, 4.0f);
    parentWidget->attachChild(childWidget);
    parentWidget->placeChild(*childWidget, Vector2(6.0f, 6.0f));
    EOKAS_EXPECT(childWidget->parent() == parentWidget.get());
    Vector2 screen = childWidget->localToScreen(Vector2(0.0f, 0.0f));
    EOKAS_EXPECT(_FloatEqual(screen.x, 6.0f));
    EOKAS_EXPECT(_FloatEqual(screen.y, 6.0f));

    UIWindow host;
    host.init(800, 600);
    auto hostRoot = std::make_shared<UIWidget>();
    hostRoot->pickable = false;
    hostRoot->shape.pivot = Vector2(0.0f, 0.0f);
    hostRoot->shape.setBox(Vector2(0.0f, 0.0f), Vector2(800.0f, 600.0f));
    auto dockHost = std::make_shared<UIDockSpace>();
    dockHost->shape.setBox(Vector2(0.0f, 40.0f), Vector2(800.0f, 560.0f));
    hostRoot->attachChild(dockHost);
    host.setRoot(hostRoot);
    UIHost dockApp;
    int created = 0;
    int destroyed = 0;
    dockApp.onCreateWindow = [&](UIWindow&, const Rect&)
    {
        created += 1;
    };
    dockApp.registerSpace(dockHost.get());
    auto makePage = [](const char* title)
    {
        auto page = std::make_shared<UIDockPage>();
        auto head = std::make_shared<UIList>();
        head->shape.setBox(Vector2(0.0f, 0.0f), Vector2(80.0f, 28.0f));
        auto label = std::make_shared<UIText>();
        label->text = title;
        label->shape.setBox(Vector2(0.0f, 0.0f), Vector2(60.0f, 16.0f));
        head->attachChild(label);
        page->setHead(head);
        auto body = std::make_shared<UIList>();
        body->shape.setBox(Vector2(0.0f, 0.0f), Vector2(200.0f, 80.0f));
        page->setBody(body);
        return page;
    };
    auto pageA = makePage("One");
    auto pageB = makePage("Two");
    dockApp.onDestroyWindow = [&](UIWindow&)
    {
        destroyed += 1;
        host.onMouseMove(420.0f, 320.0f);
        if (pageA->head() && pageA->head()->onDrag)
        {
            pageA->head()->onDrag(Vector2(12.0f, 8.0f), Vector2(40.0f, 20.0f));
        }
    };
    dockApp.observe(pageA);
    dockApp.observe(pageB);
    dockHost->dockPage(pageA, UIDockMode::Fill);
    dockHost->dockPage(pageB, UIDockMode::Fill);
    UIPrimitive prim;
    prim.begin();
    host.tick(0.0f);
    prim.end();
    UIWidget* tab = host.hitTest(20.0f, 50.0f);
    EOKAS_EXPECT(tab != nullptr);
    host.onMouseDown(20.0f, 50.0f, UIPointerCode::Left);
    host.onMouseMove(80.0f, 80.0f);
    host.onMouseMove(400.0f, 300.0f);
    host.onMouseUp(400.0f, 300.0f, UIPointerCode::Left);
    prim.begin();
    host.tick(0.0f);
    prim.end();
    EOKAS_EXPECT(created == 1);
    EOKAS_EXPECT(destroyed == 1);
    EOKAS_EXPECT(pageA->space() == dockHost.get());
    EOKAS_EXPECT(pageB->space() == dockHost.get());
    host.quit();

    int lost = 0;
    auto focused = std::make_shared<UIWidget>();
    focused->focusable = true;
    focused->onFocusLoss = [&]() { lost += 1; };
    UIWindow focusWindow;
    focusWindow.init(100, 100);
    focusWindow.setRoot(focused);
    focusWindow.setFocus(focused.get());
    EOKAS_EXPECT(focused->isFocused());
    focusWindow.setRoot(std::make_shared<UIWidget>());
    EOKAS_EXPECT(lost == 1);
    EOKAS_EXPECT(!focused->isFocused());

    auto hoverA = std::make_shared<UIWidget>();
    hoverA->shape.pivot = Vector2(0.0f, 0.0f);
    hoverA->shape.setBox(Vector2(0.0f, 0.0f), Vector2(40.0f, 40.0f));
    auto hoverB = std::make_shared<UIWidget>();
    hoverB->shape.pivot = Vector2(0.0f, 0.0f);
    hoverB->shape.setBox(Vector2(60.0f, 0.0f), Vector2(40.0f, 40.0f));
    auto hoverRoot = std::make_shared<UIWidget>();
    hoverRoot->pickable = false;
    hoverRoot->shape.pivot = Vector2(0.0f, 0.0f);
    hoverRoot->shape.setBox(Vector2(0.0f, 0.0f), Vector2(200.0f, 80.0f));
    hoverRoot->attachChild(hoverA);
    hoverRoot->attachChild(hoverB);
    UIWindow hoverWindow;
    hoverWindow.init(200, 80);
    hoverWindow.setRoot(hoverRoot);
    hoverWindow.onMouseMove(20.0f, 20.0f);
    EOKAS_EXPECT(hoverA->isHovered());
    hoverWindow.onMouseDown(20.0f, 20.0f, UIPointerCode::Left);
    hoverWindow.onMouseMove(80.0f, 20.0f);
    EOKAS_EXPECT(hoverA->isPressed());
    EOKAS_EXPECT(hoverA->isHovered());
    EOKAS_EXPECT(!hoverB->isHovered());
    hoverWindow.onMouseUp(80.0f, 20.0f, UIPointerCode::Left);
    EOKAS_EXPECT(hoverB->isHovered());

    int childWheels = 0;
    int parentWheels = 0;
    auto wheelChild = std::make_shared<UIWidget>();
    wheelChild->shape.pivot = Vector2(0.0f, 0.0f);
    wheelChild->shape.setBox(Vector2(0.0f, 0.0f), Vector2(40.0f, 40.0f));
    auto wheelParent = std::make_shared<UIWidget>();
    wheelParent->shape.pivot = Vector2(0.0f, 0.0f);
    wheelParent->shape.setBox(Vector2(0.0f, 0.0f), Vector2(80.0f, 80.0f));
    wheelParent->attachChild(wheelChild);
    wheelParent->onWheel = [&](const Vector2&, const Vector2&) { parentWheels += 1; };
    UIWindow wheelWindow;
    wheelWindow.init(80, 80);
    wheelWindow.setRoot(wheelParent);
    wheelWindow.onMouseWheel(10.0f, 10.0f, 0.0f, -24.0f);
    EOKAS_EXPECT(parentWheels == 1);
    wheelChild->onWheel = [&](const Vector2&, const Vector2&) { childWheels += 1; };
    wheelWindow.onMouseWheel(10.0f, 10.0f, 0.0f, -24.0f);
    EOKAS_EXPECT(childWheels == 1);
    EOKAS_EXPECT(parentWheels == 1);

    int anywherePress = 0;
    int anywhereRelease = 0;
    int targetedPress = 0;
    int lastAnywhere = 0;
    int lastPress = 0;
    int seq = 0;
    auto anywhereA = std::make_shared<UIWidget>();
    anywhereA->shape.pivot = Vector2(0.0f, 0.0f);
    anywhereA->shape.setBox(Vector2(0.0f, 0.0f), Vector2(40.0f, 40.0f));
    anywhereA->onPointerPressAnywhere = [&](const UIPointerEvent&) { anywherePress += 1; lastAnywhere = ++seq; };
    anywhereA->onPointerReleaseAnywhere = [&](const UIPointerEvent&) { anywhereRelease += 1; };
    anywhereA->onPointerPress = [&](const UIPointerEvent&) { targetedPress += 1; lastPress = ++seq; };
    auto anywhereB = std::make_shared<UIWidget>();
    anywhereB->shape.pivot = Vector2(0.0f, 0.0f);
    anywhereB->shape.setBox(Vector2(60.0f, 0.0f), Vector2(40.0f, 40.0f));
    auto anywhereRoot = std::make_shared<UIWidget>();
    anywhereRoot->pickable = false;
    anywhereRoot->shape.pivot = Vector2(0.0f, 0.0f);
    anywhereRoot->shape.setBox(Vector2(0.0f, 0.0f), Vector2(200.0f, 80.0f));
    anywhereRoot->attachChild(anywhereA);
    anywhereRoot->attachChild(anywhereB);
    UIWindow anywhereWindow;
    anywhereWindow.init(200, 80);
    anywhereWindow.setRoot(anywhereRoot);
    anywhereWindow.onMouseDown(70.0f, 20.0f, UIPointerCode::Left);
    EOKAS_EXPECT(anywherePress == 1);
    EOKAS_EXPECT(targetedPress == 0);
    anywhereWindow.onMouseUp(70.0f, 20.0f, UIPointerCode::Left);
    EOKAS_EXPECT(anywhereRelease == 1);
    anywhereWindow.onMouseDown(20.0f, 20.0f, UIPointerCode::Left);
    EOKAS_EXPECT(anywherePress == 2);
    EOKAS_EXPECT(targetedPress == 1);
    EOKAS_EXPECT(lastAnywhere < lastPress);
    anywhereWindow.onMouseUp(20.0f, 20.0f, UIPointerCode::Left);
    anywhereWindow.onMouseDown(150.0f, 20.0f, UIPointerCode::Left);
    EOKAS_EXPECT(anywherePress == 3);
    anywhereWindow.onMouseUp(150.0f, 20.0f, UIPointerCode::Left);
    EOKAS_EXPECT(anywhereRelease == 3);

    auto dropRoot = std::make_shared<UIWidget>();
    dropRoot->pickable = false;
    dropRoot->shape.pivot = Vector2(0.0f, 0.0f);
    dropRoot->shape.setBox(Vector2(0.0f, 0.0f), Vector2(300.0f, 200.0f));
    auto drop = std::make_shared<UIDropdown>();
    drop->shape.pivot = Vector2(0.0f, 0.0f);
    drop->shape.setBox(Vector2(0.0f, 0.0f), Vector2(120.0f, 28.0f));
    drop->addItem(1, "One");
    drop->addItem(2, "Two");
    dropRoot->attachChild(drop);
    UIWindow dropOverlayWindow;
    dropOverlayWindow.init(300, 200);
    dropOverlayWindow.setRoot(dropRoot);
    drop->setExpanded(true);
    drop->layout();
    dropOverlayWindow.onMouseDown(10.0f, 10.0f, UIPointerCode::Left);
    EOKAS_EXPECT(drop->expanded);
    dropOverlayWindow.onMouseUp(10.0f, 10.0f, UIPointerCode::Left);
    EOKAS_EXPECT(!drop->expanded);
    dropOverlayWindow.onMouseDown(10.0f, 10.0f, UIPointerCode::Left);
    dropOverlayWindow.onMouseUp(10.0f, 10.0f, UIPointerCode::Left);
    EOKAS_EXPECT(drop->expanded);
    dropOverlayWindow.onMouseDown(10.0f, 40.0f, UIPointerCode::Left);
    EOKAS_EXPECT(drop->expanded);
    dropOverlayWindow.onMouseUp(10.0f, 40.0f, UIPointerCode::Left);
    EOKAS_EXPECT(!drop->expanded);
    EOKAS_EXPECT(drop->value == 1);
    drop->setExpanded(true);
    drop->layout();
    dropOverlayWindow.onMouseDown(220.0f, 160.0f, UIPointerCode::Left);
    EOKAS_EXPECT(!drop->expanded);
    dropOverlayWindow.quit();

    auto menuRoot = std::make_shared<UIWidget>();
    menuRoot->pickable = false;
    menuRoot->shape.pivot = Vector2(0.0f, 0.0f);
    menuRoot->shape.setBox(Vector2(0.0f, 0.0f), Vector2(300.0f, 200.0f));
    auto menuHost = std::make_shared<UIWidget>();
    menuHost->shape.pivot = Vector2(0.0f, 0.0f);
    menuHost->shape.setBox(Vector2(0.0f, 0.0f), Vector2(40.0f, 20.0f));
    auto popupMenu = std::make_shared<UIMenu>();
    popupMenu->overlay = true;
    popupMenu->direction = UIDirection::Vertical;
    popupMenu->host = menuHost.get();
    popupMenu->shape.pivot = Vector2(0.0f, 0.0f);
    popupMenu->shape.setBox(Vector2(0.0f, 20.0f), Vector2(80.0f, 40.0f));
    auto popupItem = std::make_shared<UIMenuItem>();
    popupItem->setText("Item");
    popupMenu->addItem(popupItem);
    menuRoot->attachChild(menuHost);
    menuRoot->attachChild(popupMenu);
    popupMenu->layout();
    popupMenu->shape.setBox(Vector2(0.0f, 20.0f), Vector2(80.0f, 40.0f));
    UIWindow menuOverlayWindow;
    menuOverlayWindow.init(300, 200);
    menuOverlayWindow.setRoot(menuRoot);
    menuOverlayWindow.onMouseDown(10.0f, 10.0f, UIPointerCode::Left);
    EOKAS_EXPECT(popupMenu->visible);
    menuOverlayWindow.onMouseUp(10.0f, 10.0f, UIPointerCode::Left);
    menuOverlayWindow.onMouseDown(20.0f, 30.0f, UIPointerCode::Left);
    EOKAS_EXPECT(popupMenu->visible);
    menuOverlayWindow.onMouseUp(20.0f, 30.0f, UIPointerCode::Left);
    menuOverlayWindow.onMouseDown(220.0f, 160.0f, UIPointerCode::Left);
    EOKAS_EXPECT(!popupMenu->visible);
    menuOverlayWindow.quit();

    return 0;
}
