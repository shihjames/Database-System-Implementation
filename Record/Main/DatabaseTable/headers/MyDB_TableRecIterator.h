#ifndef TABLE_REC_ITER_H
#define TABLE_REC_ITER_H

#include "MyDB_RecordIterator.h"
#include "MyDB_Record.h"
#include "MyDB_TableReaderWriter.h"
#include "MyDB_Table.h"

class MyDB_TableRecIterator : public MyDB_RecordIterator
{
public:
    void getNext() override;

    bool hasNext() override;

    // constructor
    MyDB_TableRecIterator(MyDB_TableReaderWriter &, MyDB_TablePtr, MyDB_RecordPtr);

    // destructor
    ~MyDB_TableRecIterator();

private:
    // table reader/writer
    MyDB_TableReaderWriter &myTableRW;

    // table pointer
    MyDB_TablePtr myTable;

    // starting position
    MyDB_RecordPtr myRec;

    // cnt for the page during iteratipon
    int pageCnt;

    // recored iterator pointer
    MyDB_RecordIteratorPtr pageIter;
};

#endif