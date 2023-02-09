#ifndef TABLE_REC_ITER_CC
#define TABLE_REC_ITER_CC

#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableRecIterator.h"

void MyDB_TableRecIterator ::getNext()
{
    this->pageIter->getNext();
}

bool MyDB_TableRecIterator ::hasNext()
{
    if (this->pageIter->hasNext())
    {
        return true;
    }

    while (this->pageCnt < this->myTable->lastPage())
    {
        this->pageCnt += 1;
        this->pageIter = this->myTableRW[this->pageCnt].getIterator(this->myRec);
        if (this->pageIter->hasNext())
        {
            return true;
        }
    }

    return false;
}

MyDB_TableRecIterator ::MyDB_TableRecIterator(MyDB_TableReaderWriter &tableRWIn, MyDB_TablePtr tableIn, MyDB_RecordPtr recIn) : myTableRW(tableRWIn), myTable(tableIn), myRec(recIn)
{
    pageCnt = 0;
    pageIter = myTableRW[pageCnt].getIterator(myRec);
}

MyDB_TableRecIterator ::~MyDB_TableRecIterator() {}

#endif