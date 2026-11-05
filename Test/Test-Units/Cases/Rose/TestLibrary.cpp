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

EOKAS_TEST_CASE(edits) {
    Library library("edits");
    Schema* text = library.getSchema("String");
    Schema* integer = library.getSchema("Int");
    Schema* row = library.addSchema(SchemaType::Struct, "Row");
    EOKAS_EXPECT(row->insertMember(0, "name", text));
    EOKAS_EXPECT(row->insertMember(0, "age", integer));
    EOKAS_EXPECT(row->getMemberIndex("age") == 0);
    EOKAS_EXPECT(row->renameMember("age", "years"));
    EOKAS_EXPECT(row->getMember("years") != nullptr);
    EOKAS_EXPECT(!row->insertMember(0, "years", integer));

    Schema* listSchema = library.addSchema(SchemaType::List, "Rows");
    listSchema->setElementSchema(row);
    Value* list = library.make(listSchema);
    Value* first = library.make(row);
    Value* middle = library.make(row);
    Value* last = library.make(row);
    EOKAS_EXPECT(library.push(list, first));
    EOKAS_EXPECT(library.push(list, last));
    EOKAS_EXPECT(library.insert(list, 1, middle));
    EOKAS_EXPECT(library.count(list) == 3);

    Value* name = library.get(middle, "name");
    EOKAS_EXPECT(library.setNull(name));
    EOKAS_EXPECT(library.isNull(name));
    String got;
    EOKAS_EXPECT(!library.get(name, got));
    EOKAS_EXPECT(library.set(name, String("mid")));
    EOKAS_EXPECT(!library.isNull(name));
    EOKAS_EXPECT(library.get(name, got));
    EOKAS_EXPECT(got == "mid");

    Value* years = library.get(middle, "years");
    EOKAS_EXPECT(library.setNull(years));
    EOKAS_EXPECT(library.isNull(years));

    EOKAS_EXPECT(library.renameField(first, "name", "title"));
    EOKAS_EXPECT(library.get(first, "title", got));
    Value* note = library.make(text);
    EOKAS_EXPECT(library.set(note, String("extra")));
    EOKAS_EXPECT(row->insertMember(2, "note", text));
    EOKAS_EXPECT(library.put(first, "note", note));
    EOKAS_EXPECT(library.get(first, "note", got));
    EOKAS_EXPECT(got == "extra");
    EOKAS_EXPECT(library.removeField(first, "note"));
    EOKAS_EXPECT(library.remove(list, 0));
    EOKAS_EXPECT(library.count(list) == 2);

    library.set("rows", list);
    MemoryStream memory;
    EOKAS_EXPECT(memory.open());
    BinaryStream stream(memory);
    EOKAS_EXPECT(library.save(stream));
    EOKAS_EXPECT(stream.seek(0, 0));
    Library loaded("loaded-edits");
    EOKAS_EXPECT(loaded.load(stream));
    Value* loadedRows = loaded.get("rows");
    EOKAS_EXPECT(loaded.count(loadedRows) == 2);
    Value* loadedMiddle = loaded.get(loadedRows, 0);
    String loadedName;
    EOKAS_EXPECT(loaded.get(loadedMiddle, "name", loadedName));
    EOKAS_EXPECT(loadedName == "mid");
    Value* loadedYears = loaded.get(loadedMiddle, "years");
    EOKAS_EXPECT(loaded.isNull(loadedYears));
    EOKAS_EXPECT(!loaded.get(loadedYears, got));
    return 0;
}
