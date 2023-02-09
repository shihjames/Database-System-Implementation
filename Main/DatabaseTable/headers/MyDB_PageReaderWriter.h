
#ifndef PAGE_RW_H
#define PAGE_RW_H

#include "MyDB_PageType.h"
#include "MyDB_TableReaderWriter.h"

// structure for page overlay
struct PageOverlay
{
	// offset for next unwritten byte
	unsigned offsetToNextUnwritten;
	// type of the page
	MyDB_PageType pageType;
	// this is where the data will be
	char bytes[0];
};

class MyDB_PageReaderWriter
{

public:
	// empties out the contents of this page, so that it has no records in it
	// the type of the page is set to MyDB_PageType :: RegularPage
	void clear();

	// return an itrator over this page... each time returnVal->next () is
	// called, the resulting record will be placed into the record pointed to
	// by iterateIntoMe
	MyDB_RecordIteratorPtr getIterator(MyDB_RecordPtr iterateIntoMe);

	// appends a record to this page... return false is the append fails because
	// there is not enough space on the page; otherwise, return true
	bool append(MyDB_RecordPtr appendMe);

	// gets the type of this page... this is just a value from an ennumeration
	// that is stored within the page
	MyDB_PageType getType();

	// sets the type of the page
	void setType(MyDB_PageType toMe);

	// constructor
	MyDB_PageReaderWriter(MyDB_BufferManagerPtr myBufferMgr, MyDB_TablePtr myTablePtr, long myPageID);

private:
	// pointer to the buffer manager (provided by TableReaderWriter)
	MyDB_BufferManagerPtr myBufferMgr;

	// pointer to the buffer manager (provided by TableReaderWriter)
	MyDB_TablePtr myTablePtr;

	// page id (provided by TableReaderWriter)
	long myPageID;

	// page handle for handling wroteBytes() and getBytes()
	MyDB_PageHandle myPageHandle;

	// structure of page overlay
	PageOverlay *myPageOverlay;
};

#endif
