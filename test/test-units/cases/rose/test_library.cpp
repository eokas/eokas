#include "Unit.h"
#include "rose/library.h"

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
    people->setElement(person);
    EOKAS_EXPECT(people->getElement() == person);
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
    listSchema->setElement(intSchema);
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
