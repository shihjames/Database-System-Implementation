
#ifndef PAGE_RW_C
#define PAGE_RW_C

#include <algorithm>
#include "MyDB_PageReaderWriter.h"
#include "MyDB_PageRecIterator.h"
#include "MyDB_PageRecIteratorAlt.h"
#include "MyDB_PageListIteratorAlt.h"
#include "RecordComparator.h"

#define NUM_BYTES_USED *((size_t *)(myPageOverlay->offsetToNextUnwritten + sizeof(PageOverlay)))

void MyDB_PageReaderWriter ::clear()
{
	// set offsetToNextUnwritten to index 0
	myPageOverlay->offsetToNextUnwritten = 0;
	// set page type to RegularPage
	setType(MyDB_PageType ::RegularPage);
	// set dirty
	myPageHandle->wroteBytes();
}

MyDB_RecordIteratorPtr MyDB_PageReaderWriter ::getIterator(MyDB_RecordPtr myRecPtr)
{
	// create a share pointer to page record iterator
	return make_shared<MyDB_PageRecIterator>(myPageHandle, myRecPtr);
}

MyDB_RecordIteratorAltPtr MyDB_PageReaderWriter ::getIteratorAlt()
{
	return make_shared<MyDB_PageRecIteratorAlt>(myPageHandle);
}

MyDB_RecordIteratorAltPtr getIteratorAlt(vector<MyDB_PageReaderWriter> &forUs)
{
	return make_shared<MyDB_PageListIteratorAlt>(forUs);
}

MyDB_PageType MyDB_PageReaderWriter ::getType()
{
	return myPageOverlay->pageType;
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

void *MyDB_PageReaderWriter ::appendAndReturnLocation(MyDB_RecordPtr appendMe)
{
	void *recLocation = myPageOverlay->offsetToNextUnwritten + sizeof(PageOverlay) + (char *)myPageHandle->getBytes();
	if (append(appendMe))
		return recLocation;
	else
		return nullptr;
}

void MyDB_PageReaderWriter ::
	sortInPlace(function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs)
{

	void *temp = malloc(pageSize);
	memcpy(temp, myPageHandle->getBytes(), pageSize);

	// first, read in the positions of all of the records
	vector<void *> positions;

	// this basically iterates through all of the records on the page
	int bytesConsumed = sizeof(size_t) * 2;
	while (bytesConsumed != myPageOverlay->offsetToNextUnwritten + sizeof(PageOverlay))
	{
		void *pos = bytesConsumed + (char *)temp;
		positions.push_back(pos);
		void *nextPos = lhs->fromBinary(pos);
		bytesConsumed += ((char *)nextPos) - ((char *)pos);
	}

	// and now we sort the vector of positions, using the record contents to build a comparator
	RecordComparator myComparator(comparator, lhs, rhs);
	std::stable_sort(positions.begin(), positions.end(), myComparator);

	// and write the guys back
	NUM_BYTES_USED = 2 * sizeof(size_t);
	myPageHandle->wroteBytes();
	for (void *pos : positions)
	{
		lhs->fromBinary(pos);
		append(lhs);
	}

	free(temp);
}

MyDB_PageReaderWriterPtr MyDB_PageReaderWriter ::
	sort(function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs)
{

	// first, read in the positions of all of the records
	vector<void *> positions;

	// this basically iterates through all of the records on the page
	int bytesConsumed = sizeof(size_t) * 2;
	while (bytesConsumed != NUM_BYTES_USED)
	{
		void *pos = bytesConsumed + (char *)myPageHandle->getBytes();
		positions.push_back(pos);
		void *nextPos = lhs->fromBinary(pos);
		bytesConsumed += ((char *)nextPos) - ((char *)pos);
	}

	// and now we sort the vector of positions, using the record contents to build a comparator
	RecordComparator myComparator(comparator, lhs, rhs);
	std::stable_sort(positions.begin(), positions.end(), myComparator);

	// and now create the page to return
	MyDB_PageReaderWriterPtr returnVal = make_shared<MyDB_PageReaderWriter>(myBufferMgr);
	returnVal->clear();

	// loop through all of the sorted records and write them out
	for (void *pos : positions)
	{
		lhs->fromBinary(pos);
		returnVal->append(lhs);
	}

	return returnVal;
}

size_t MyDB_PageReaderWriter ::getPageSize()
{
	return pageSize;
}

void *MyDB_PageReaderWriter ::getBytes()
{
	return myPageHandle->getBytes();
}

MyDB_PageReaderWriter ::MyDB_PageReaderWriter(MyDB_BufferManagerPtr bufferMgrIn, MyDB_TablePtr tablePtrIn, long pageIDIn) : myBufferMgr(bufferMgrIn), myTablePtr(tablePtrIn), myPageID(pageIDIn)
{
	// initialize page handler from getPage (non-anonymous, unpinned page)
	this->myPageHandle = myBufferMgr->getPage(myTablePtr, myPageID);
	// initialize pageoverlay from raw bytes
	myPageOverlay = (PageOverlay *)myPageHandle->getBytes();
	// get page size
	pageSize = myBufferMgr->getPageSize();
}

MyDB_PageReaderWriter ::MyDB_PageReaderWriter(bool pinned, MyDB_BufferManagerPtr bufferMgrIn, MyDB_TablePtr &tablePtrIn, long pageIDIn) : myBufferMgr(bufferMgrIn), myTablePtr(tablePtrIn), myPageID(pageIDIn)
{
	if (pinned)
	{
		// initialize page handler from getPinnedPage (non-anonymous, pinned page)
		this->myPageHandle = myBufferMgr->getPinnedPage(myTablePtr, myPageID);
	}
	else
	{
		// initialize page handler from getPage (non-anonymous, unpinned page)
		this->myPageHandle = myBufferMgr->getPage(myTablePtr, myPageID);
	}
	// initialize pageoverlay from raw bytes
	myPageOverlay = (PageOverlay *)myPageHandle->getBytes();
	// get page size
	pageSize = myBufferMgr->getPageSize();
}

MyDB_PageReaderWriter ::MyDB_PageReaderWriter(MyDB_BufferManagerPtr bufferMgrIn) : myBufferMgr(bufferMgrIn)
{
	// initialize page handler from getPage (anonymous, unpinned page)
	this->myPageHandle = myBufferMgr->getPage(myTablePtr, myPageID);
	// initialize pageoverlay from raw bytes
	myPageOverlay = (PageOverlay *)myPageHandle->getBytes();
	// get page size
	pageSize = myBufferMgr->getPageSize();
	clear();
}

MyDB_PageReaderWriter ::MyDB_PageReaderWriter(bool pinned, MyDB_BufferManagerPtr bufferMgrIn) : myBufferMgr(bufferMgrIn)
{
	if (pinned)
	{
		this->myPageHandle = myBufferMgr->getPinnedPage();
	}
	else
	{
		this->myPageHandle = myBufferMgr->getPage();
	}
	pageSize = myBufferMgr->getPageSize();
	clear();
}

#endif
