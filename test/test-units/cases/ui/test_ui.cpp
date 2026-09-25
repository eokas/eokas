#include "Unit.h"
#include "ui/main.h"

using namespace eokas;

EOKAS_TEST_CASE(ui) {
    int clicks = 0;
    UIWidget widget;
    widget.rect = Rect(0, 0, 40, 20);
    widget.onClick = [&]() { clicks += 1; };
    widget.triggerClick();
    EOKAS_EXPECT(clicks == 1);
    widget.triggerPointerEnter();
    EOKAS_EXPECT(widget.hovered);
    widget.triggerPointerLeave();
    EOKAS_EXPECT(!widget.hovered);

    UIList list;
    auto first = std::make_shared<UIWidget>();
    auto second = std::make_shared<UIWidget>();
    first->rect = Rect(0, 0, 10, 20);
    second->rect = Rect(0, 0, 10, 30);
    list.addChild(first);
    list.addChild(second);
    list.layout(Rect(0, 0, 100, 200));
    EOKAS_EXPECT(_FloatEqual(first->rect.x, 8.0f));
    EOKAS_EXPECT(_FloatEqual(first->rect.y, 8.0f));
    EOKAS_EXPECT(_FloatEqual(second->rect.y, 36.0f));

    UIGrid grid;
    grid.columns = 2;
    grid.cellWidth = 16.0f;
    grid.cellHeight = 10.0f;
    auto cell = std::make_shared<UIWidget>();
    cell->rect = Rect(0, 0, 16, 10);
    grid.addChild(cell);
    grid.layout(Rect(0, 0, 80, 40));
    EOKAS_EXPECT(_FloatEqual(cell->rect.width, 16.0f));

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
    body->rect = Rect(0, 0, 12, 8);
    side->rect = Rect(0, 0, 10, 6);
    table.setContent(1, 0, body);
    table.setContent(0, 1, side);
    table.refit();
    EOKAS_EXPECT(_FloatEqual(table.rect.width, 68.0f));
    EOKAS_EXPECT(_FloatEqual(table.rect.height, 46.0f));
    table.layout(Rect(0, 0, 0, 0));
    EOKAS_EXPECT(_FloatEqual(body->rect.x, 5.0f));
    EOKAS_EXPECT(_FloatEqual(body->rect.y, 33.0f));
    table.layout(Rect(0, 0, 100, 0));
    EOKAS_EXPECT(_FloatEqual(table.cell(0, 1)->rect.x, 48.0f));
    EOKAS_EXPECT(_FloatEqual(table.cell(0, 1)->rect.width, 52.0f));
    EOKAS_EXPECT(_FloatEqual(side->rect.x, 53.0f));
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
    EOKAS_EXPECT(child->parent() == group);
    EOKAS_EXPECT(child->depth() == 1);
    EOKAS_EXPECT(child->index() == 0);
    EOKAS_EXPECT(grand->depth() == 2);
    auto mark = std::make_shared<UIWidget>();
    auto other = std::make_shared<UIWidget>();
    mark->rect = Rect(0, 0, 12, 8);
    other->rect = Rect(0, 0, 6, 6);
    child->setContent(0, mark);
    child->setContent(1, other);
    tree.layout(Rect(0, 0, 0, 0));
    EOKAS_EXPECT(_FloatEqual(tree.rect.width, 120.0f));
    EOKAS_EXPECT(_FloatEqual(tree.rect.height, 60.0f));
    EOKAS_EXPECT(_FloatEqual(mark->rect.x, 20.0f));
    EOKAS_EXPECT(_FloatEqual(mark->rect.y, 20.0f));
    EOKAS_EXPECT(_FloatEqual(other->rect.x, 80.0f));
    int leafHits = 0;
    grand->onExpandedChanged = [&](bool) { leafHits += 1; };
    grand->triggerClick();
    EOKAS_EXPECT(grand->expanded());
    EOKAS_EXPECT(leafHits == 0);
    child->setExpanded(false);
    EOKAS_EXPECT(!child->expanded());
    EOKAS_EXPECT(_FloatEqual(tree.rect.height, 40.0f));
    EOKAS_EXPECT(tree.children.size() == 2);
    child->setExpanded(true);
    group->setExpanded(false);
    EOKAS_EXPECT(child->expanded());
    EOKAS_EXPECT(_FloatEqual(tree.rect.height, 20.0f));
    EOKAS_EXPECT(tree.children.size() == 1);
    group->setExpanded(true);
    int hits = 0;
    group->onExpandedChanged = [&](bool) { hits += 1; };
    group->triggerClick();
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
    box->rect = Rect(0, 0, 12, 8);
    only->setContent(0, box);
    wide.refit();
    EOKAS_EXPECT(_FloatEqual(wide.rect.width, 22.0f));
    EOKAS_EXPECT(_FloatEqual(wide.rect.height, 8.0f));

    UIButton button;
    button.setText("OK");
    EOKAS_EXPECT(button.label() != nullptr);
    EOKAS_EXPECT(button.label()->text == "OK");

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
    toggle.triggerClick();
    EOKAS_EXPECT(toggle.value);
    EOKAS_EXPECT(toggleHits == 1);

    UIInput input;
    input.setText("eokas");
    EOKAS_EXPECT(input.text == "eokas");
    EOKAS_EXPECT(input.acceptsKeyFocus());

    UIDropdown dropdown;
    dropdown.addItem(1, "One");
    dropdown.addItem(2, "Two");
    dropdown.setValue(2);
    EOKAS_EXPECT(dropdown.value == 2);
    EOKAS_EXPECT(dropdown.labelOf(2) == "Two");

    UIShape shape;
    shape.begin();
    shape.addQuad(Rect(0, 0, 10, 10), Rect(0, 0, 1, 1), Color(1, 1, 1, 1));
    shape.end();
    EOKAS_EXPECT(shape.indexCount == 6);
    EOKAS_EXPECT(shape.vertexLength == 6 * sizeof(UIVertex));

    UIFrame frame;
    frame.init(200, 100);
    auto root = std::make_shared<UIWidget>();
    root->rect = Rect(0, 0, 200, 100);
    frame.setRoot(root);
    EOKAS_EXPECT(frame.hitTest(10, 10) == root.get());
    frame.onMouseDown(10, 10, 0);
    frame.onMouseUp(10, 10, 0);
    frame.quit();

    UIView view;
    auto content = std::make_shared<UIWidget>();
    content->rect = Rect(0, 0, 400, 300);
    view.addChild(content);
    view.rect = Rect(0, 0, 50, 40);
    view.setScroll(4, 6);
    EOKAS_EXPECT(_FloatEqual(view.scrollX(), 4.0f));
    EOKAS_EXPECT(_FloatEqual(view.scrollY(), 6.0f));

    UIDockSpace dock;
    dock.layout(Rect(0, 0, 400, 300));
    auto page = std::make_shared<UIDockPage>();
    dock.dockPage(page, UIDockMode::Fill);
    dock.layout(Rect(0, 0, 400, 300));
    EOKAS_EXPECT(page->mode() == UIDockMode::Fill);
    EOKAS_EXPECT(page->space() == &dock);
    Rect bounds;
    EOKAS_EXPECT(dock.pageBounds(page.get(), bounds));
    EOKAS_EXPECT(bounds.width > 0.0f);
    EOKAS_EXPECT(bounds.height > 0.0f);

    int windows = 0;
    UIApp app;
    app.onCreateWindow = [&](const Rect&) {
        windows += 1;
        return (void*)1;
    };
    void* window = (void*)1;
    UIFrame& opened = app.open(window, 320, 240);
    EOKAS_EXPECT(app.find(window) == &opened);
    app.layout(window, 320, 240);
    app.close(window);
    EOKAS_EXPECT(app.find(window) == nullptr);
    return 0;
}
