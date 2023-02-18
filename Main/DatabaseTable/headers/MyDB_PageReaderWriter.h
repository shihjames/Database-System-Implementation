
#ifndef PAGE_RW_H
#define PAGE_RW_H

#include <memory>
#include "MyDB_PageType.h"
#include "MyDB_RecordIterator.h"
#include "MyDB_RecordIteratorAlt.h"
#include "MyDB_TableReaderWriter.h"

using namespace std;

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

class MyDB_PageReaderWriter;
typedef shared_ptr<MyDB_PageReaderWriter> MyDB_PageReaderWriterPtr;

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

	// gets an instance of an alternate iterator over the page... this is an
	// iterator that has the alternate getCurrent ()/advance () interface
	MyDB_RecordIteratorAltPtr getIteratorAlt();

	// gets an instance of an alternatie iterator over a list of pages
	friend MyDB_RecordIteratorAltPtr getIteratorAlt(vector<MyDB_PageReaderWriter> &forUs);

	// appends a record to this page... return false is the append fails because
	// there is not enough space on the page; otherwise, return true
	bool append(MyDB_RecordPtr appendMe);

	// appends a record to this page... return a pointer to the location of where
	// the record is written if there is enough space on the page; otherwise, return
	// a nullptr
	void *appendAndReturnLocation(MyDB_RecordPtr appendMe);

	// gets the type of this page... this is just a value from an ennumeration
	// that is stored within the page
	MyDB_PageType getType();

	// sets the type of the page
	void setType(MyDB_PageType toMe);

	// sorts the contents of the page... the boolean lambda that is sent into
	// this function must check to see if the contents of the record pointed to
	// by lhs are less than the contens of the record pointed to by rhs... typically,
	// this lambda would have been created via a call to buildRecordComparator
	MyDB_PageReaderWriterPtr sort(function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs);

	// like the above, except that the sorting is done in place, on the page
	void sortInPlace(function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs);

	// returns the page size
	size_t getPageSize();

	// returns the actual bytes
	void *getBytes();

	// constructor for a page in the same file as the parent
	MyDB_PageReaderWriter(MyDB_BufferManagerPtr myBufferMgr, MyDB_TablePtr myTablePtr, long myPageID);

	// constructor for a page that can be pinned, if desired
	MyDB_PageReaderWriter(bool pinned, MyDB_BufferManagerPtr myBufferMgr, MyDB_TablePtr &myTablePtr, long myPageID);

	// constructor for an anonymous page
	MyDB_PageReaderWriter(MyDB_BufferManagerPtr myBufferMgr);

	// constructor for an anonymous page that can be pinned, if desired
	MyDB_PageReaderWriter(bool pinned, MyDB_BufferManagerPtr myBufferMgr);

private:
	// pointer to the buffer manager (provided by TableReaderWriter)
	MyDB_BufferManagerPtr myBufferMgr;

	// pointer to the buffer manager (provided by TableReaderWriter)
	MyDB_TablePtr myTablePtr;

	// page id (provided by TableReaderWriter)
	long myPageID;

	// page size
	size_t pageSize;

	// page handle for handling wroteBytes() and getBytes()
	MyDB_PageHandle myPageHandle;

	// structure of page overlay
	PageOverlay *myPageOverlay;
};

#endif
