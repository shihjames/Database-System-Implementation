#ifndef PAGE_REC_ITER_CC
#define PAGE_REC_ITER_CC

#include "MyDB_PageRecIterator.h"

void MyDB_PageRecIterator ::getNext()
{
    // first check if has next
    if (hasNext())
    {
        PageOverlay *pageOverlay = (PageOverlay *)myPageHandle->getBytes();
        // read
        myRecPtr->fromBinary(&(pageOverlay->bytes[myOffset]));
        // update myOffset
        myOffset += myRecPtr->getBinarySize();
    }
}

bool MyDB_PageRecIterator ::hasNext()
{
    PageOverlay *pageOverlay = (PageOverlay *)myPageHandle->getBytes();
    // next record exists
    if (myOffset < pageOverlay->offsetToNextUnwritten)
    {
        return true;
    }
    else
    {
        return false;
    }
}

MyDB_PageRecIterator ::MyDB_PageRecIterator(MyDB_PageHandle pageHandleIn, MyDB_RecordPtr recPtrIn)
{
    myPageHandle = pageHandleIn;
    myRecPtr = recPtrIn;
    myOffset = 0;
}

MyDB_PageRecIterator ::~MyDB_PageRecIterator() {}

#endif