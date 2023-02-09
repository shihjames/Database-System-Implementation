# Database System Implementation

Author: James Shih

## Project Part 2 Description

Reads and writes records from pages, and then uses the buffer manager to store and retrieve those pages.

1. MyDB_Record objects

    1. can read and write records from a string of bytes
    2. can evaluate functions over records
    3. can manage table schemas and create records from schemas
    4. can save meta-data information (such as schemas) to and retrieve information from the catalog file, among other things.

2. MyDB_Record objects are going to be used is built into the interface of the MyDB_PageReaderWriter and MyDB_TableReaderWriter classes.

3. Iteration is accomplished via the MyDB_RecordIterator class.

## Usage

1. Initialize:

```cpp
MyDB_CatalogPtr myCatalog = make_shared<MyDB_Catalog>("catFile");
// make a schema
MyDB_SchemaPtr mySchema = make_shared<MyDB_Schema>();
mySchema->appendAtt(make_pair("suppkey", make_shared<MyDB_IntAttType>()));
mySchema->appendAtt(make_pair("name", make_shared<MyDB_StringAttType>()));
mySchema->appendAtt(make_pair("address", make_shared<MyDB_StringAttType>()));
mySchema->appendAtt(make_pair("nationkey", make_shared<MyDB_IntAttType>()));
mySchema->appendAtt(make_pair("phone", make_shared<MyDB_StringAttType>()));
mySchema->appendAtt(make_pair("acctbal", make_shared<MyDB_DoubleAttType>()));
mySchema->appendAtt(make_pair("comment", make_shared<MyDB_StringAttType>()));

// use the schema to create a table
MyDB_TablePtr myTable = make_shared<MyDB_Table>("supplier", "supplier.bin", mySchema);
MyDB_BufferManagerPtr myMgr = make_shared<MyDB_BufferManager>(1024, 16, "tempFile");
MyDB_TableReaderWriter supplierTable(myTable, myMgr);

// load it from a text file
supplierTable.loadFromTextFile("supplier.tbl");

// put the supplier table into the catalog
myTable->putInCatalog(myCatalog);
```

2. Append Records:

```cpp
// create manager
MyDB_CatalogPtr myCatalog = make_shared<MyDB_Catalog>("catFile");
map<string, MyDB_TablePtr> allTables = MyDB_Table::getAllTables(myCatalog);
MyDB_BufferManagerPtr myMgr = make_shared<MyDB_BufferManager>(1024, 16, "tempFile");

// create TableReaderWriter
MyDB_TableReaderWriter supplierTable(allTables["supplier"], myMgr);
MyDB_RecordPtr temp = supplierTable.getEmptyRecord();

// generate record
string s = "10001|Supplier#000010001|00000000|999|12-345-678-9012|1234.56|the special record|";
temp->fromString(s);

// append record
supplierTable.append(temp);
```
