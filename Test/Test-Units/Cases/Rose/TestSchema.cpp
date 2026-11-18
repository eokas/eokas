#include "Unit.h"
#include "Rose/Schema.h"

using namespace eokas;
using namespace eokas::rose;

EOKAS_TEST_CASE(schema_del_member_option) {
    SchemaHeap heap;
    Schema* text = heap.get("String");
    Schema* integer = heap.get("Int");

    Schema* row = heap.add(SchemaType::Struct, "Row");
    EOKAS_EXPECT(row->addMember("id", text));
    EOKAS_EXPECT(row->addMember("qty", integer));
    EOKAS_EXPECT(row->getMemberCount() == 2);
    row->delMember("id");
    EOKAS_EXPECT(row->getMemberCount() == 1);
    EOKAS_EXPECT(row->getMember("id") == nullptr);
    EOKAS_EXPECT(row->getMember("qty") != nullptr);
    EOKAS_EXPECT(row->getMemberIndex("qty") == 0);

    EOKAS_EXPECT(row->setMember("note", text));
    EOKAS_EXPECT(row->getMemberCount() == 2);
    EOKAS_EXPECT(row->getMember("note")->schema == text);

    Schema* status = heap.add(SchemaType::Options, "Status");
    EOKAS_EXPECT(status->addOption("a"));
    EOKAS_EXPECT(status->addOption("b"));
    status->delOption("a");
    EOKAS_EXPECT(status->getOptionCount() == 1);
    EOKAS_EXPECT(status->getOption(0)->name == "b");
    return 0;
}

EOKAS_TEST_CASE(schema_list_element_no_self) {
    SchemaHeap heap;
    Schema* list = heap.add(SchemaType::List, "SelfList");
    list->setElementSchema(list);
    EOKAS_EXPECT(list->getElementSchema() == nullptr);
    Schema* integer = heap.get("Int");
    list->setElementSchema(integer);
    EOKAS_EXPECT(list->getElementSchema() == integer);
    return 0;
}
