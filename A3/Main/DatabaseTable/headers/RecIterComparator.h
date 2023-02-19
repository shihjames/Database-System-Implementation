
#ifndef REC_ITER_COMPARATOR_H
#define REC_ITER_COMPARATOR_H

#include "MyDB_Record.h"
#include "MyDB_RecordIteratorAlt.h"
#include <iostream>
using namespace std;

class RecIterComparator
{

public:
    RecIterComparator(function<bool()> comp, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs)
    {
        comparator = comp;
        this->lhs = lhs;
        this->rhs = rhs;
    }

    bool operator()(MyDB_RecordIteratorAltPtr iter1, MyDB_RecordIteratorAltPtr iter2)
    {
        iter1->getCurrent(lhs);
        iter2->getCurrent(rhs);
        return !comparator();
    }

private:
    function<bool()> comparator;
    MyDB_RecordPtr lhs;
    MyDB_RecordPtr rhs;
};

#endif