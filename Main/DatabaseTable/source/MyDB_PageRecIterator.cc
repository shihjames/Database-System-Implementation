

/****************************************************
** COPYRIGHT 2016, Chris Jermaine, Rice University **
**                                                 **
** The MyDB Database System, COMP 530              **
** Note that this file contains SOLUTION CODE for  **
** A2.  You should not be looking at this file     **
** unless you have completed A2!                   **
****************************************************/

#ifndef PAGE_REC_ITER_C
#define PAGE_REC_ITER_C

#include "MyDB_PageRecIterator.h"
#include "MyDB_PageType.h"

#define NUM_BYTES_USED *((size_t *)(((char *)myPageHandle->getBytes()) + sizeof(size_t)))

void MyDB_PageRecIterator ::getNext()
{
    void *pos = myOffset + (char *)myPageHandle->getBytes();
    void *nextPos = myRecPtr->fromBinary(pos);
    myOffset += ((char *)nextPos) - ((char *)pos);
}

void *MyDB_PageRecIterator ::getCurrentPointer()
{
    return myOffset + (char *)myPageHandle->getBytes();
}

bool MyDB_PageRecIterator ::hasNext()
{
    return myOffset != NUM_BYTES_USED;
}

MyDB_PageRecIterator ::MyDB_PageRecIterator(MyDB_PageHandle myPageIn, MyDB_RecordPtr myRecIn)
{
    myOffset = sizeof(size_t) * 2;
    myPageHandle = myPageIn;
    myRecPtr = myRecIn;
}

MyDB_PageRecIterator ::~MyDB_PageRecIterator() {}

#endif