
#ifndef PAGE_REC_ITER_H
#define PAGE_REC_ITER_H

#include "MyDB_PageReaderWriter.h"
#include "MyDB_PageHandle.h"
#include "MyDB_Record.h"
#include "MyDB_RecordIterator.h"

class MyDB_PageRecIterator : public MyDB_RecordIterator
{

public:
    void getNext() override;

    bool hasNext() override;

    // BEFORE a call to getNext (), a call to getCurrentPointer () will get the address
    // of the record.  At a later time, it is then possible to reconstitute the record
    // by calling MyDB_Record.fromBinary (obtainedPointer)... ASSUMING that the page
    // that the record is located on has not been swapped out
    void *getCurrentPointer() override;

    // constructor
    MyDB_PageRecIterator(MyDB_PageHandle myPageHandle, MyDB_RecordPtr myRecPtr);

    // destructor
    ~MyDB_PageRecIterator();

private:
    // page handler for handling getbBytes() and wroteBytes() (provided by PageReaderWriter)
    MyDB_PageHandle myPageHandle;

    // pointer to the record (provided by PageReaderWriter)
    MyDB_RecordPtr myRecPtr;

    // current offset of unwritten bytes
    size_t myOffset;
};

#endif