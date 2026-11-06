#include "Unit.h"
#include "Rose/Library.h"

using namespace eokas;
using namespace eokas::rose;

EOKAS_TEST_CASE(schema) {
    SchemaHeap heap;
    EOKAS_EXPECT(heap.count() == 4);
    Schema* integer = heap.get("Int");
    Schema* text = heap.get("String");
    EOKAS_EXPECT(integer != nullptr);
    EOKAS_EXPECT(text != nullptr);
    EOKAS_EXPECT(heap.add(SchemaType::Int, "Int") == nullptr);
    EOKAS_EXPECT(heap.get("Int") == integer);
    EOKAS_EXPECT(heap.indexOf("String") == 3);
    EOKAS_EXPECT(*integer == *heap.get(0));
    EOKAS_EXPECT(*integer != *text);

    Schema* person = heap.add(SchemaType::Struct, "Person");
    person->addMember("name", text);
    person->addMember("age", integer);
    EOKAS_EXPECT(person->getMemberCount() == 2);
    EOKAS_EXPECT(person->getMember("name")->schema == text);
    EOKAS_EXPECT(person->getMemberIndex("age") == 1);

    Schema* people = heap.add(SchemaType::List, "People");
    people->setElementSchema(person);
    EOKAS_EXPECT(people->getElementSchema() == person);
    EOKAS_EXPECT(heap.count() == 6);
    return 0;
}

EOKAS_TEST_CASE(library) {
    SchemaHeap schemas;
    ValueHeap values(schemas);
    Value* made = values.make(42);
    i32_t stored = 0;
    EOKAS_EXPECT(values.get(made, stored));
    EOKAS_EXPECT(stored == 42);
    EOKAS_EXPECT(values.set(made, 7));
    EOKAS_EXPECT(values.get(made, stored));
    EOKAS_EXPECT(stored == 7);
    EOKAS_EXPECT(values.indexOf(made) == 0);

    Library library("demo");
    library.set("answer", 42);
    library.set("name", String("eokas"));
    library.set("ok", true);
    i32_t answer = 0;
    String name;
    bool ok = false;
    EOKAS_EXPECT(library.get("answer", answer));
    EOKAS_EXPECT(answer == 42);
    EOKAS_EXPECT(library.get("name", name));
    EOKAS_EXPECT(name == "eokas");
    EOKAS_EXPECT(library.get("ok", ok));
    EOKAS_EXPECT(ok);

    Schema* listSchema = library.addSchema(SchemaType::List, "Ints");
    Schema* intSchema = library.getSchema("Int");
    EOKAS_EXPECT(intSchema != nullptr);
    listSchema->setElementSchema(intSchema);
    Value* list = library.make(listSchema);
    EOKAS_EXPECT(library.push(list, 3));
    EOKAS_EXPECT(library.push(list, 4));
    i32_t popped = 0;
    EOKAS_EXPECT(library.pop(list, popped));
    EOKAS_EXPECT(popped == 4);

    MemoryStream memory;
    EOKAS_EXPECT(memory.open());
    BinaryStream stream(memory);
    EOKAS_EXPECT(library.save(stream));
    EOKAS_EXPECT(stream.seek(0, 0));
    Library loaded("loaded");
    EOKAS_EXPECT(loaded.load(stream));
    i32_t loadedAnswer = 0;
    EOKAS_EXPECT(loaded.get("answer", loadedAnswer));
    EOKAS_EXPECT(loadedAnswer == 42);
    return 0;
}

EOKAS_TEST_CASE(list_struct) {
    Library library("list-struct");
    Schema* integer = library.getSchema("Int");
    Schema* text = library.getSchema("String");
    Schema* listSchema = library.addSchema(SchemaType::List, "Ints");
    listSchema->setElementSchema(integer);
    Value* list = library.make(listSchema);
    EOKAS_EXPECT(library.push(list, 1));
    EOKAS_EXPECT(library.push(list, 2));
    Value* first = library.get(list, 0);
    EOKAS_EXPECT(library.insert(list, 0, first));
    EOKAS_EXPECT(library.count(list) == 3);
    i32_t got = 0;
    EOKAS_EXPECT(library.get(list, 0, got));
    EOKAS_EXPECT(got == 1);
    EOKAS_EXPECT(library.get(list, 1, got));
    EOKAS_EXPECT(got == 1);
    EOKAS_EXPECT(library.get(list, 2, got));
    EOKAS_EXPECT(got == 2);

    Value* extra = library.make(9);
    EOKAS_EXPECT(!library.insert(list, 4, extra));
    EOKAS_EXPECT(!library.remove(list, 3));
    EOKAS_EXPECT(library.clear(list));
    EOKAS_EXPECT(library.count(list) == 0);

    Schema* row = library.addSchema(SchemaType::Struct, "Row");
    row->addMember("name", text);
    Value* record = library.make(row);
    EOKAS_EXPECT(!library.clear(record));
    EOKAS_EXPECT(library.get(record, "name") != nullptr);
    EOKAS_EXPECT(!library.set(record, "missing", extra));
    EOKAS_EXPECT(library.get(record, "name") != nullptr);
    return 0;
}

EOKAS_TEST_CASE(row_string_list) {
    Library library("row-string-list");
    Schema* text = library.getSchema("String");
    Schema* tags = library.addSchema(SchemaType::List, "Tags");
    tags->setElementSchema(text);
    Schema* row = library.addSchema(SchemaType::Struct, "Row");
    row->addMember("tags", tags);
    Value* record = library.make(row);
    Value* list = library.get(record, "tags");
    EOKAS_EXPECT(library.push(list, String("red")));
    EOKAS_EXPECT(library.push(list, String("blue")));
    EOKAS_EXPECT(library.count(list) == 2);

    library.set("row", record);
    MemoryStream memory;
    EOKAS_EXPECT(memory.open());
    BinaryStream stream(memory);
    EOKAS_EXPECT(library.save(stream));
    EOKAS_EXPECT(stream.seek(0, 0));
    Library loaded("loaded-row-string-list");
    EOKAS_EXPECT(loaded.load(stream));
    Value* loadedRow = loaded.get("row");
    Value* loadedTags = loaded.get(loadedRow, "tags");
    EOKAS_EXPECT(loaded.count(loadedTags) == 2);
    String first;
    String second;
    EOKAS_EXPECT(loaded.get(loadedTags, 0, first));
    EOKAS_EXPECT(loaded.get(loadedTags, 1, second));
    EOKAS_EXPECT(first == "red");
    EOKAS_EXPECT(second == "blue");
    return 0;
}

EOKAS_TEST_CASE(struct_member_list_grows) {
    Library library("struct-member-list-grows");
    Schema* text = library.getSchema("String");
    Schema* option = library.addSchema(SchemaType::Struct, "Option");
    option->addMember("label", text);
    option->addMember("color", text);
    Schema* options = library.addSchema(SchemaType::List, "Options");
    options->setElementSchema(option);
    Schema* column = library.addSchema(SchemaType::Struct, "Column");
    column->addMember("options", options);
    Value* record = library.make(column);
    Value* list = library.get(record, "options");
    EOKAS_EXPECT(list != nullptr);
    for (int count = 1; count <= 32; ++count)
    {
        EOKAS_EXPECT(library.clear(list));
        for (int i = 0; i < count; ++i)
        {
            Value* item = library.make(option);
            EOKAS_EXPECT(library.set(item, "label", String::format("选项%d", i + 1)));
            EOKAS_EXPECT(library.set(item, "color", String("#6366f1")));
            EOKAS_EXPECT(library.push(list, item));
        }
        EOKAS_EXPECT(library.count(list) == static_cast<u32_t>(count));
    }
    String last;
    EOKAS_EXPECT(library.get(library.get(list, 31), "label", last));
    EOKAS_EXPECT(last == "选项32");
    return 0;
}

