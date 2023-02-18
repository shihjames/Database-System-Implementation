
#ifndef TABLE_RW_C
#define TABLE_RW_C

#include <iostream>
#include <fstream>
#include <limits>
#include <queue>
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableRecIterator.h"
#include "MyDB_TableRecIteratorAlt.h"
#include "MyDB_TableReaderWriter.h"
#include <set>
#include <vector>
// #include "Sorting.h"

using namespace std;

MyDB_TableReaderWriter ::MyDB_TableReaderWriter(MyDB_TablePtr tablePtr, MyDB_BufferManagerPtr mgrPtr)
{
	this->myTable = tablePtr;
	this->myBuffer = mgrPtr;

	// if it is a whole new table, set lastpage to 0 as initialization
	if (this->myTable->lastPage() == -1)
	{
		this->myTable->setLastPage(0);
		this->lastPage = make_shared<MyDB_PageReaderWriter>(this->myBuffer, this->myTable, this->myTable->lastPage());
		this->lastPage->clear();
	}
	// if it is an existing table, just create the last page pagereaderwriter shared_ptr
	else
	{
		this->lastPage = make_shared<MyDB_PageReaderWriter>(this->myBuffer, this->myTable, this->myTable->lastPage());
	}
}

// get i-th pages
MyDB_PageReaderWriter MyDB_TableReaderWriter ::operator[](size_t i)
{
	return *make_shared<MyDB_PageReaderWriter>(this->myBuffer, this->myTable, i);
}

MyDB_RecordPtr MyDB_TableReaderWriter ::getEmptyRecord()
{
	return make_shared<MyDB_Record>(this->myTable->getSchema());
}

MyDB_PageReaderWriter MyDB_TableReaderWriter ::last()
{
	return *make_shared<MyDB_PageReaderWriter>(this->myBuffer, this->myTable, this->myTable->lastPage());
}

void MyDB_TableReaderWriter ::append(MyDB_RecordPtr appendRec)
{
	// successfully append a record in page
	if (this->lastPage->append(appendRec))
	{
		// cout << "success append!" << endl;
	}
	// insufficient space to append record to current last page, add a new page to this table
	else
	{
		this->myTable->setLastPage(myTable->lastPage() + 1);
		this->lastPage = make_shared<MyDB_PageReaderWriter>(this->myBuffer, this->myTable, this->myTable->lastPage());

		// Q: do we need to initialize the new page we just created here ? A: yes, use clear()
		this->lastPage->clear();
		this->lastPage->append(appendRec);
	}
}
// for unitest, use to load data from existing data.
void MyDB_TableReaderWriter ::loadFromTextFile(string filename)
{

	this->myTable->setLastPage(0);
	this->lastPage = make_shared<MyDB_PageReaderWriter>(this->myBuffer, this->myTable, this->myTable->lastPage());
	this->lastPage->clear();

	ifstream file(filename);
	if (file.is_open())
	{
		string line;
		MyDB_RecordPtr rec = getEmptyRecord();
		while (getline(file, line))
		{
			rec->fromString(line);
			append(rec);
		}
		file.close();
	}
	else
	{
		cout << "Failed to open file: " << filename.c_str() << endl;
	}
}

MyDB_RecordIteratorPtr MyDB_TableReaderWriter ::getIterator(MyDB_RecordPtr recIter)
{
	return make_shared<MyDB_TableRecIterator>(*this, myTable, recIter);
}

MyDB_RecordIteratorAltPtr MyDB_TableReaderWriter ::getIteratorAlt()
{
	return make_shared<MyDB_TableRecIteratorAlt>(*this, myTable);
}

MyDB_RecordIteratorAltPtr MyDB_TableReaderWriter ::getIteratorAlt(int lowPage, int highPage)
{
	return make_shared<MyDB_TableRecIteratorAlt>(*this, myTable, lowPage, highPage);
}

void MyDB_TableReaderWriter ::writeIntoTextFile(string filename)
{
	// possible for future unit test
	ofstream file;
	file.open(filename);
	// open file
	if (file.is_open())
	{
		MyDB_RecordPtr recPtr = getEmptyRecord();
		MyDB_RecordIteratorPtr iterPtr = getIterator(recPtr);

		while (iterPtr->hasNext())
		{
			iterPtr->getNext();
			// wrire text into file
			file << recPtr << endl;
		}

		file.close();
	}
	else
	{
		cout << "Failed to open file: " << filename.c_str() << endl;
	}
}

MyDB_BufferManagerPtr MyDB_TableReaderWriter ::getBufferMgr()
{
	return this->myBuffer;
}

MyDB_TablePtr MyDB_TableReaderWriter ::getTable()
{
	return this->myTable;
}

#endif
