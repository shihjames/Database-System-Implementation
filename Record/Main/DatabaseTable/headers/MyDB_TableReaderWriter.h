
#ifndef TABLE_RW_H
#define TABLE_RW_H

#include <memory>
#include "../../BufferMgr/headers/MyDB_BufferManager.h"
#include "MyDB_Record.h"
#include "MyDB_RecordIterator.h"
// #include "MyDB_TableRecIterator.h"
#include "MyDB_Table.h"
#include "MyDB_PageReaderWriter.h"

using namespace std;

// create a smart pointer for the catalog
class MyDB_TableReaderWriter;
class MyDB_PageReaderWriter;
// shared pointer of TableReaderWriter and PageReaderWriter
typedef shared_ptr<MyDB_TableReaderWriter> MyDB_TableReaderWriterPtr;
typedef shared_ptr<MyDB_PageReaderWriter> MyDB_PageReaderWriterPtr;

class MyDB_TableReaderWriter
{

public:
	// create a table reader/writer for the specified table, using the specified
	// buffer manager
	MyDB_TableReaderWriter(MyDB_TablePtr, MyDB_BufferManagerPtr);

	// gets an empty record from this table
	MyDB_RecordPtr getEmptyRecord();

	// append a record to the table
	void append(MyDB_RecordPtr appendMe);

	// return an itrator over this table... each time returnVal->next () is
	// called, the resulting record will be placed into the record pointed to
	// by iterateIntoMe
	MyDB_RecordIteratorPtr getIterator(MyDB_RecordPtr iterateIntoMe);

	// load a text file into this table... overwrites the current contents
	void loadFromTextFile(string fromMe);

	// dump the contents of this table into a text file
	void writeIntoTextFile(string toMe);

	// access the i^th page in this file
	MyDB_PageReaderWriter operator[](size_t i);

	// access the last page in the file
	MyDB_PageReaderWriter last();

	// get pointer of buffer manager
	MyDB_BufferManagerPtr getBufferMgr();

	// get table pointer
	MyDB_TablePtr getTable();

private:
	friend class MyDB_pageReaderWriter;

	// shared pointer of PageReaderWriter for the last page
	shared_ptr<MyDB_PageReaderWriter> lastPage;

	// pointer to the table (provided by TableReaderWriter)
	MyDB_TablePtr myTable;

	// pointer to the buffer manager (provided by TableReaderWriter)
	MyDB_BufferManagerPtr myBuffer;
};

#endif