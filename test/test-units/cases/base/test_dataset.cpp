#include "Unit.h"

using namespace eokas;

EOKAS_TEST_CASE(dataset) {
    DataSet set;
    DataTable* table = set.createTable("people");
    EOKAS_EXPECT(set.tableCount() == 1);
    EOKAS_EXPECT(set.containsTable("people"));
    EOKAS_EXPECT(set.selectTable("people") == table);
    EOKAS_EXPECT(set.createTable("people") == nullptr);

    DataCol* name = table->createCol("name");
    name->setType(eDataType_String);
    name->setComm("display name");
    EOKAS_EXPECT(table->colCount() == 1);
    EOKAS_EXPECT(table->containsCol("name"));
    EOKAS_EXPECT(name->type() == eDataType_String);
    EOKAS_EXPECT(name->comm() == "display name");

    DataRow row = table->createRow();
    row["name"] = String("eokas");
    EOKAS_EXPECT(table->rowCount() == 1);
    EOKAS_EXPECT(table->selectRow(0)["name"].operator String() == "eokas");

    table->deleteRow(0);
    EOKAS_EXPECT(table->rowCount() == 0);
    set.deleteTable("people");
    EOKAS_EXPECT(set.tableCount() == 0);
    return 0;
}
