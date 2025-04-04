
#include "./dataset.h"
#include "./string.h"

namespace eokas {
    /*
    ============================================================================================
    ==== DataCell
    ============================================================================================
    */
    DataCell::DataCell()
        : mLength(0), mData(nullptr) {
    }
    
    DataCell::~DataCell() {
        this->clear();
    }
    
    u8_t* DataCell::data() const {
        return mData;
    }
    
    u16_t DataCell::length() const {
        return mLength;
    }
    
    void DataCell::setData(u8_t* data, u16_t length) {
        this->clear();
        mData = new u8_t[length];
        memcpy(mData, data, length);
        mLength = length;
    }
    
    void DataCell::clear() {
        mLength = 0;
        if (mData != nullptr) {
            delete[]mData;
            mData = nullptr;
        }
    }
    
    DataCell& DataCell::operator=(const DataCell& cell) {
        if (this != &cell) {
            this->setData(cell.mData, cell.mLength);
        }
        return *this;
    }
    
    DataCell& DataCell::operator=(const String& value) {
        this->setData((u8_t*) value.cstr(), (u16_t) value.length());
        return *this;
    }
    
    DataCell& DataCell::operator=(char value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell& DataCell::operator=(u8_t value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell& DataCell::operator=(i16_t value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell& DataCell::operator=(u16_t value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell& DataCell::operator=(i32_t value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell& DataCell::operator=(u32_t value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell& DataCell::operator=(i64_t value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell& DataCell::operator=(u64_t value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell& DataCell::operator=(f32_t value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell& DataCell::operator=(f64_t value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell& DataCell::operator=(bool value) {
        this->setData((u8_t*) &value, sizeof(value));
        return *this;
    }
    
    DataCell::operator String() {
        char* cstr = new char[mLength + 1];
        memcpy(cstr, mData, mLength);
        cstr[mLength] = '\0';
        return cstr;
    }
    
    DataCell::operator char() {
        return *(char*) mData;
    }
    
    DataCell::operator u8_t() {
        return *(u8_t*) mData;
    }
    
    DataCell::operator i16_t() {
        return *(i16_t*) mData;
    }
    
    DataCell::operator u16_t() {
        return *(u16_t*) mData;
    }
    
    DataCell::operator i32_t() {
        return *(i32_t*) mData;
    }
    
    DataCell::operator u32_t() {
        return *(u32_t*) mData;
    }
    
    DataCell::operator i64_t() {
        return *(i64_t*) mData;
    }
    
    DataCell::operator u64_t() {
        return *(u64_t*) mData;
    }
    
    DataCell::operator f32_t() {
        return *(f32_t*) mData;
    }
    
    DataCell::operator f64_t() {
        return *(f64_t*) mData;
    }
    
    DataCell::operator bool() {
        return *(bool*) mData;
    }
    
    /*
    ============================================================================================
    ==== DataRow
    ============================================================================================
    */
    DataRow::DataRow()
        : mCells() {
    }
    
    DataRow::~DataRow() {
    }
    
    DataCell& DataRow::operator[](const String& colName) {
        return *(this->getCell(colName));
    }
    
    void DataRow::setCell(const String& colName, DataCell* cell) {
        mCells[colName] = cell;
    }
    
    DataCell* DataRow::getCell(const String& colName) {
        auto cellIter = mCells.find(colName);
        if (cellIter == mCells.end())
            return nullptr;
        return cellIter->second;
    }
    
    /*
    ============================================================================================
    ==== DataCol
    ============================================================================================
    */
    DataCol::DataCol(const String& name)
        : mName(name), mComm(), mType(eDataType_Raw), mLength(0), mCells() {
    }
    
    DataCol::~DataCol() {
        this->clearCells();
    }
    
    const String& DataCol::name() const {
        return mName;
    }
    
    const String& DataCol::comm() const {
        return mComm;
    }
    
    u16_t DataCol::length() const {
        return mLength;
    }
    
    const DataType& DataCol::type() const {
        return mType;
    }
    
    void DataCol::setComm(const String& comm) {
        mComm = comm;
    }
    
    void DataCol::setLength(u16_t length) {
        mLength = length;
    }
    
    void DataCol::setType(const DataType& type) {
        mType = type;
    }
    
    DataCell* DataCol::createCell() {
        DataCell* cell = new DataCell();
        mCells.push_back(cell);
        return cell;
    }
    
    DataCell* DataCol::selectCell(size_t rowId) {
        if (rowId >= mCells.size())
            return nullptr;
        return mCells[rowId];
    }
    
    void DataCol::deleteCell(size_t rowId) {
        if (rowId >= mCells.size())
            return;
        if (rowId == 0) {
            DataCell* cell = mCells.front();
            if (cell != nullptr) {
                delete cell;
            }
            mCells.pop_front();
        } else if (rowId == mCells.size()) {
            DataCell* cell = mCells.back();
            if (cell != nullptr) {
                delete cell;
            }
            mCells.pop_back();
        } else if (rowId < mCells.size() / 2) {
            auto cellIter = mCells.begin();
            for (u32_t i = 0; i < rowId; ++i) {
                ++cellIter;
            }
            DataCell* cell = *cellIter;
            if (cell != nullptr) {
                delete cell;
            }
            mCells.erase(cellIter);
        } else {
            auto cellIter = mCells.end();
            for (size_t i = mCells.size(); i > rowId; --i) {
                --cellIter;
            }
            DataCell* cell = *cellIter;
            if (cell != nullptr) {
                delete cell;
            }
            mCells.erase(cellIter);
        }
    }
    
    void DataCol::clearCells() {
        auto cellIter = mCells.begin();
        while (cellIter != mCells.end()) {
            DataCell* cell = *cellIter;
            if (cell != nullptr) {
                delete cell;
            }
            ++cellIter;
        }
        mCells.clear();
    }
    
    
    /*
    ============================================================================================
    ==== DataTable
    ============================================================================================
    */
    DataTable::DataTable(const String& name)
        : mName(name), mComm(), mCols(), mRowCount(0) {
    }
    
    DataTable::~DataTable() {
        auto colIter = mCols.begin();
        while (colIter != mCols.end()) {
            DataCol* col = colIter->second;
            if (col != nullptr) {
                delete col;
            }
            ++colIter;
        }
        mCols.clear();
    }
    
    DataCol& DataTable::operator[](const String& colName) {
        if (this->containsCol(colName)) {
            return *(this->selectCol(colName));
        }
        return *(this->createCol(colName));
    }
    
    DataRow DataTable::operator[](size_t rowId) {
        return this->selectRow(rowId);
    }
    
    bool DataTable::containsCol(const String& colName) {
        return mCols.find(colName) != mCols.end();
    }
    
    const String& DataTable::name() const {
        return mName;
    }
    
    const String& DataTable::comm() const {
        return mComm;
    }
    
    void DataTable::setComm(const String& comm) {
        mComm = comm;
    }
    
    size_t DataTable::colCount() const {
        return mCols.size();
    }
    
    size_t DataTable::rowCount() const {
        return mRowCount;
    }
    
    DataCol* DataTable::createCol(const String& colName) {
        if (mCols.find(colName) != mCols.end())
            return nullptr;
        DataCol* col = new DataCol(colName);
        mCols[colName] = col;
        return col;
    }
    
    DataCol* DataTable::selectCol(const String& colName) {
        auto colIter = mCols.find(colName);
        if (colIter == mCols.end())
            return nullptr;
        return colIter->second;
    }
    
    void DataTable::deleteCol(const String& colName) {
        auto colIter = mCols.find(colName);
        if (colIter != mCols.end()) {
            DataCol* col = colIter->second;
            if (col != nullptr) {
                delete col;
            }
            mCols.erase(colIter);
        }
    }
    
    DataRow DataTable::createRow() {
        DataRow row;
        auto colIter = mCols.begin();
        while (colIter != mCols.end()) {
            DataCol* col = colIter->second;
            if (col != nullptr) {
                DataCell* cell = col->createCell();
                row.setCell(col->name(), cell);
            }
            
            ++colIter;
        }
        mRowCount += 1;
        return row;
    }
    
    DataRow DataTable::selectRow(size_t rowId) {
        DataRow row;
        if (rowId < mRowCount) {
            auto colIter = mCols.begin();
            while (colIter != mCols.end()) {
                DataCol* col = colIter->second;
                if (col != nullptr) {
                    DataCell* cell = col->selectCell(rowId);
                    row.setCell(col->name(), cell);
                }
                
                ++colIter;
            }
        }
        return row;
    }
    
    void DataTable::deleteRow(size_t rowId) {
        if (rowId < mRowCount) {
            auto colIter = mCols.begin();
            while (colIter != mCols.end()) {
                DataCol* col = colIter->second;
                if (col != nullptr) {
                    col->deleteCell(rowId);
                }
                
                ++colIter;
            }
            mRowCount -= 1;
        }
    }
    
    void DataTable::clear() {
        auto colIter = mCols.begin();
        while (colIter != mCols.end()) {
            DataCol* col = colIter->second;
            if (col != nullptr) {
                col->clearCells();
            }
            
            ++colIter;
        }
        mRowCount = 0;
    }
    
    /*
    ============================================================================================
    ==== DataSet
    ============================================================================================
    */
    DataSet::DataSet()
        : mVersion(_RadixDataSetVersion), mTables() {
    }
    
    DataSet::~DataSet() {
        auto tableIter = mTables.begin();
        while (tableIter != mTables.end()) {
            DataTable* table = tableIter->second;
            if (table != nullptr) {
                delete table;
            }
            ++tableIter;
        }
        mTables.clear();
    }
    
    DataTable& DataSet::operator[](const String& tableName) {
        if (this->containsTable(tableName)) {
            return *(this->selectTable(tableName));
        }
        return *(this->createTable(tableName));
    }
    
    size_t DataSet::tableCount() const {
        return mTables.size();
    }
    
    bool DataSet::containsTable(const String& tableName) {
        return mTables.find(tableName) != mTables.end();
    }
    
    DataTable* DataSet::createTable(const String& tableName) {
        if (mTables.find(tableName) != mTables.end())
            return nullptr;
        DataTable* table = new DataTable(tableName);
        mTables[tableName] = table;
        return table;
    }
    
    DataTable* DataSet::selectTable(const String& tableName) {
        auto tableIter = mTables.find(tableName);
        if (tableIter == mTables.end())
            return nullptr;
        return tableIter->second;
    }
    
    void DataSet::deleteTable(const String& tableName) {
        auto tableIter = mTables.find(tableName);
        if (tableIter != mTables.end()) {
            DataTable* table = tableIter->second;
            if (table != nullptr) {
                delete table;
            }
            mTables.erase(tableIter);
        }
    }
    
    void DataSet::load(u8_t* bytes) {
        if (bytes == nullptr)
            return;
        
        u8_t* ptr = bytes;
        
        mVersion = *((u32_t*) ptr);
        ptr += 4;
        
        u16_t tableCount = *((u16_t*) ptr);
        ptr += 2;
        for (u16_t tableId = 0; tableId < tableCount; tableId++) {
            u16_t tableNameLength = *((u16_t*) ptr);
            ptr += 2;
            char* tableNameBuffer = new char[tableNameLength + 1];
            memcpy(tableNameBuffer, ptr, tableNameLength);
            tableNameBuffer[tableNameLength] = '\0';
            String tableName = tableNameBuffer;
            delete[]tableNameBuffer;
            ptr += tableNameLength;
            
            u16_t tableCommLength = *((u16_t*) ptr);
            ptr += 2;
            char* tableCommBuffer = new char[tableCommLength + 1];
            memcpy(tableCommBuffer, ptr, tableCommLength);
            tableCommBuffer[tableCommLength] = '\0';
            String tableComm = tableCommBuffer;
            delete[]tableCommBuffer;
            ptr += tableCommLength;
            
            DataTable* table = this->createTable(tableName);
            table->setComm(tableComm);
            
            u16_t colCount = *((u16_t*) ptr);
            ptr += 2;
            u16_t rowCount = *((u16_t*) ptr);
            ptr += 2;
            
            table->mRowCount = rowCount;
            for (u16_t colId = 0; colId < colCount; colId++) {
                u16_t colNameLength = *((u16_t*) ptr);
                ptr += 2;
                char* colNameBuffer = new char[colNameLength + 1];
                memcpy(colNameBuffer, ptr, colNameLength);
                colNameBuffer[colNameLength] = '\0';
                String colName = colNameBuffer;
                delete[]colNameBuffer;
                ptr += colNameLength;
                
                u16_t colCommLength = *((u16_t*) ptr);
                ptr += 2;
                char* colCommBuffer = new char[colCommLength + 1];
                memcpy(colCommBuffer, ptr, colCommLength);
                colCommBuffer[colCommLength] = '\0';
                String colComm = colCommBuffer;
                delete[]colCommBuffer;
                ptr += colCommLength;
                
                u16_t colLength = *((u16_t*) ptr);
                ptr += 2;
                u16_t colType = *((u16_t*) ptr);
                ptr += 2;
                
                DataCol* col = table->createCol(colName);
                col->setComm(colComm);
                col->setLength(colLength);
                col->setType((DataType) colType);
                
                for (u16_t rowId = 0; rowId < rowCount; rowId++) {
                    u16_t cellLength = *((u16_t*) ptr);
                    ptr += 2;
                    u8_t* cellData = new u8_t[cellLength];
                    memcpy(cellData, ptr, cellLength);
                    ptr += cellLength;
                    
                    DataCell* cell = col->createCell();
                    cell->setData(cellData, cellLength);
                }
            }
        }
    }
    
    void DataSet::save(u8_t* bytes, size_t* length) {
        if (bytes == nullptr)
            return;
        
        u8_t* ptr = bytes;
        
        (*(u32_t*) ptr) = mVersion;
        ptr += 4;
        
        (*(u16_t*) ptr) = (u16_t) this->tableCount();
        ptr += 2;
        auto tableIter = mTables.begin();
        while (tableIter != mTables.end()) {
            DataTable* table = tableIter->second;
            if (table != nullptr) {
                String tableName = table->name().cstr();
                (*(u16_t*) ptr) = (u16_t) tableName.length();
                ptr += 2;
                memcpy(ptr, tableName.cstr(), tableName.length());
                ptr += tableName.length();
                
                String tableComm = table->comm();
                (*(u16_t*) ptr) = (u16_t) tableComm.length();
                ptr += 2;
                memcpy(ptr, tableComm.cstr(), tableComm.length());
                ptr += tableComm.length();
                
                (*(u16_t*) ptr) = (u16_t) table->colCount();;
                ptr += 2;
                (*(u16_t*) ptr) = (u16_t) table->rowCount();
                ptr += 2;
                
                auto colIter = table->mCols.begin();
                while (colIter != table->mCols.end()) {
                    DataCol* col = colIter->second;
                    if (col != nullptr) {
                        String colName = col->name();
                        (*(u16_t*) ptr) = (u16_t) colName.length();
                        ptr += 2;
                        memcpy(ptr, colName.cstr(), colName.length());
                        ptr += colName.length();
                        
                        String colComm = col->comm();
                        (*(u16_t*) ptr) = (u16_t) colComm.length();
                        ptr += 2;
                        memcpy(ptr, colComm.cstr(), colComm.length());
                        ptr += colComm.length();
                        
                        (*(u16_t*) ptr) = (u16_t) col->length();
                        ptr += 2;
                        (*(u16_t*) ptr) = (u16_t) col->type();
                        ptr += 2;
                        
                        for (u16_t rowId = 0; rowId < table->rowCount(); rowId++) {
                            DataCell* cell = col->mCells[rowId];
                            if (cell != nullptr) {
                                (*(u16_t*) ptr) = (u16_t) cell->length();
                                ptr += 2;
                                memcpy(ptr, cell->data(), cell->length());
                                ptr += cell->length();
                            } else {
                                (*(u16_t*) ptr) = 0;
                                ptr += 2;
                            }
                        }
                    }
                    ++colIter;
                }
            }
            ++tableIter;
        }
        *length = ptr - bytes;
    }
    
    void DataSet::clear() {
        auto tableIter = mTables.begin();
        while (tableIter != mTables.end()) {
            DataTable* table = tableIter->second;
            if (table == nullptr) {
                tableIter = mTables.erase(tableIter);
                continue;
            }
            table->clear();
            
            ++tableIter;
        }
    }
    
}
