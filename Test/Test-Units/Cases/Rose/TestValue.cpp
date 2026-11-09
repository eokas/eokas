#include "Unit.h"
#include "Rose/Schema.h"
#include "Rose/Value.h"

using namespace eokas;
using namespace eokas::rose;

EOKAS_TEST_CASE(value_heap_primitives) {
    SchemaHeap schemas;
    ValueHeap values(schemas);

    Value* flt = values.make(3.5);
    f64_t f = 0;
    EOKAS_EXPECT(values.get(flt, f));
    EOKAS_EXPECT(f == 3.5);
    EOKAS_EXPECT(values.set(flt, -1.25));
    EOKAS_EXPECT(values.get(flt, f));
    EOKAS_EXPECT(f == -1.25);

    Value* big = values.make(i64_t(9000000000000LL));
    i64_t i64 = 0;
    EOKAS_EXPECT(values.get(big, i64));
    EOKAS_EXPECT(i64 == 9000000000000LL);

    Value* flag = values.make(true);
    bool b = false;
    EOKAS_EXPECT(values.get(flag, b));
    EOKAS_EXPECT(b);
    EOKAS_EXPECT(values.set(flag, false));
    EOKAS_EXPECT(values.get(flag, b));
    EOKAS_EXPECT(!b);

    Value* str = values.make(String("alpha"));
    String got;
    EOKAS_EXPECT(values.get(str, got));
    EOKAS_EXPECT(got == "alpha");
    EOKAS_EXPECT(values.set(str, String("alpha")));
    EOKAS_EXPECT(values.get(str, got));
    EOKAS_EXPECT(got == "alpha");
    u32_t idxBefore = str->value.u64;
    EOKAS_EXPECT(values.set(str, String("beta")));
    EOKAS_EXPECT(values.get(str, got));
    EOKAS_EXPECT(got == "beta");
    EOKAS_EXPECT(str->value.u64 != idxBefore);
    return 0;
}

EOKAS_TEST_CASE(value_heap_schema_dependency) {
    SchemaHeap schemas;
    ValueHeap values(schemas);
    Schema* text = schemas.get("String");
    Schema* integer = schemas.get("Int");
    Schema* orphan = schemas.add(SchemaType::Struct, "Orphan");

    Schema* row = schemas.add(SchemaType::Struct, "Row");
    row->addMember("name", text);
    row->addMember("age", integer);

    Value* record = values.make(row);
    EOKAS_EXPECT(values.doesValueDependOnSchema(record, row));
    EOKAS_EXPECT(values.doesValueDependOnSchema(record, text));
    EOKAS_EXPECT(values.doesValueDependOnSchema(record, integer));
    EOKAS_EXPECT(!values.doesValueDependOnSchema(record, orphan));
    EOKAS_EXPECT(!values.doesValueDependOnSchema(record, nullptr));

    Schema* nums = schemas.add(SchemaType::List, "Nums");
    nums->setElementSchema(integer);
    Value* list = values.make(nums);
    Value* n = values.make(1);
    values.push(list, n);
    EOKAS_EXPECT(values.doesValueDependOnSchema(list, nums));
    EOKAS_EXPECT(values.doesValueDependOnSchema(list, integer));
    return 0;
}
