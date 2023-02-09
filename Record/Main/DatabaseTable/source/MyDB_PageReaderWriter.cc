
#ifndef PAGE_RW_C
#define PAGE_RW_C

#include "MyDB_PageReaderWriter.h"
#include "MyDB_PageRecIterator.h"

void MyDB_PageReaderWriter ::clear()
{
	// set offsetToNextUnwritten to index 0
	myPageOverlay->offsetToNextUnwritten = 0;
	// set page type to RegularPage
	setType(MyDB_PageType ::RegularPage);
	// set dirty
	myPageHandle->wroteBytes();
}

MyDB_PageType MyDB_PageReaderWriter ::getType()
{
	return myPageOverlay->pageType;
}

MyDB_RecordIteratorPtr MyDB_PageReaderWriter ::getIterator(MyDB_RecordPtr myRecPtr)
{
	// create a share pointer to page record iterator
	return make_shared<MyDB_PageRecIterator>(myPageHandle, myRecPtr);
}

void MyDB_PageReaderWriter ::setType(MyDB_PageType pageTypeIn)
{
	// set new page type
	myPageOverlay->pageType = pageTypeIn;
	// set dirty
	myPageHandle->wroteBytes();
}

bool MyDB_PageReaderWriter ::append(MyDB_RecordPtr myRecordPtr)
{
	// get the record size from record pointer
	size_t recordSize = myRecordPtr->getBinarySize();
	// get the page size from buffer manager
	size_t pageSize = this->myBufferMgr->getPageSize();
	// count remaining size
	size_t remainSize = pageSize - myPageOverlay->offsetToNextUnwritten - sizeof(PageOverlay);
	// able to append record
	if (recordSize < remainSize)
	{
		// write
		myRecordPtr->toBinary(&(myPageOverlay->bytes[(myPageOverlay->offsetToNextUnwritten)]));
		// update offsetToNextUnwritten
		myPageOverlay->offsetToNextUnwritten += recordSize;
		// set dirty
		myPageHandle->wroteBytes();
		return true;
	}
	// unable to append record
	else
	{
		return false;
	}
}

MyDB_PageReaderWriter ::MyDB_PageReaderWriter(MyDB_BufferManagerPtr bufferMgrIn, MyDB_TablePtr tablePtrIn, long pageIDIn) : myBufferMgr(bufferMgrIn), myTablePtr(tablePtrIn), myPageID(pageIDIn)
{
	// initialize page handler from getPage (non-anonymous, unpinned page)
	this->myPageHandle = myBufferMgr->getPage(myTablePtr, myPageID);
	// initialize pageoverlay from raw bytes
	myPageOverlay = (PageOverlay *)myPageHandle->getBytes();
}

#endif
