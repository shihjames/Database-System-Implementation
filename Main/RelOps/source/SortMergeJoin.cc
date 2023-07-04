
#ifndef SORTMERGE_CC
#define SORTMERGE_CC

#include "Aggregate.h"
#include "MyDB_Record.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"
#include "SortMergeJoin.h"
#include "Sorting.h"

SortMergeJoin ::SortMergeJoin(MyDB_TableReaderWriterPtr leftInputIn, MyDB_TableReaderWriterPtr rightInputIn,
                              MyDB_TableReaderWriterPtr outputIn, string finalSelectionPredicateIn,
                              vector<string> projectionsIn,
                              pair<string, string> equalityCheckIn, string leftSelectionPredicateIn,
                              string rightSelectionPredicateIn)
{

    output = outputIn;
    finalSelectionPredicate = finalSelectionPredicateIn;
    projections = projectionsIn;
    equalityCheck = equalityCheckIn;
    leftTable = leftInputIn;
    rightTable = rightInputIn;
    leftSelectionPredicate = leftSelectionPredicateIn;
    rightSelectionPredicate = rightSelectionPredicateIn;
}

void SortMergeJoin ::run()
{

    // initialize runsize
    int runSize = int(leftTable->getBufferMgr()->numPages / 2);

    MyDB_RecordPtr leftRecPtr = leftTable->getEmptyRecord();
    MyDB_RecordPtr leftRecPtr2 = leftTable->getEmptyRecord();
    MyDB_RecordPtr rightRecPtr = rightTable->getEmptyRecord();
    MyDB_RecordPtr rightRecPtr2 = rightTable->getEmptyRecord();

    // comparator for both tables
    function<bool()> leftComp = buildRecordComparator(leftRecPtr, leftRecPtr2, equalityCheck.first);
    function<bool()> rightComp = buildRecordComparator(rightRecPtr, rightRecPtr2, equalityCheck.second);

    // sort both tables and get the iterators
    MyDB_RecordIteratorAltPtr leftIter = buildItertorOverSortedRuns(runSize, *leftTable, leftComp, leftRecPtr, leftRecPtr2, leftSelectionPredicate);
    MyDB_RecordIteratorAltPtr rightIter = buildItertorOverSortedRuns(runSize, *rightTable, rightComp, rightRecPtr, rightRecPtr2, rightSelectionPredicate);

    // and get the schema that results from combining the left and right records
    MyDB_SchemaPtr mySchemaOut = make_shared<MyDB_Schema>();
    for (auto &p : leftTable->getTable()->getSchema()->getAtts())
        mySchemaOut->appendAtt(p);
    for (auto &p : rightTable->getTable()->getSchema()->getAtts())
        mySchemaOut->appendAtt(p);

    // get the combined record
    MyDB_RecordPtr combinedRec = make_shared<MyDB_Record>(mySchemaOut);
    combinedRec->buildFrom(leftRecPtr, rightRecPtr); //(all records are in combinedRec now)

    // now, get the final predicate over it
    func finalPredicate = combinedRec->compileComputation(finalSelectionPredicate); //(give it the selection condition which will be used later)

    // and get the final set of computatoins that will be used to buld the output record
    vector<func> finalComputations; //(build up the projction functions and put it into final computations)
    for (string s : projections)
    {
        finalComputations.push_back(combinedRec->compileComputation(s));
    }

    // this is the output record
    MyDB_RecordPtr outputRec = output->getEmptyRecord();

    // MERGE PHASE

    vector<MyDB_PageReaderWriter> leftVecRec;
    vector<MyDB_PageReaderWriter> rightVecRec;
    MyDB_PageReaderWriter leftPage(true, *(leftTable->getBufferMgr()));
    MyDB_PageReaderWriter rightPage(true, *(rightTable->getBufferMgr()));

    function<bool()> leftCompReverse = buildRecordComparator(leftRecPtr2, leftRecPtr, equalityCheck.first);

    func left = combinedRec->compileComputation(" < (" + equalityCheck.first + ", " + equalityCheck.second + ")");
    func right = combinedRec->compileComputation(" > (" + equalityCheck.first + ", " + equalityCheck.second + ")");
    func equal = combinedRec->compileComputation(" == (" + equalityCheck.first + ", " + equalityCheck.second + ")");

    if (leftIter->advance() && rightIter->advance())
    {
        while (true)
        {

            // check if one of iterator has reach to the end
            bool flag = false;

            leftIter->getCurrent(leftRecPtr);
            rightIter->getCurrent(rightRecPtr);

            if (left()->toBool())
            {
                if (!leftIter->advance())
                    flag = true;
            }
            else if (right()->toBool())
            {
                if (!rightIter->advance())
                    flag = true;
            }
            else if (equal()->toBool())
            {
                leftPage.clear();
                leftVecRec.clear();
                leftVecRec.push_back(leftPage);

                rightPage.clear();
                rightVecRec.clear();
                rightVecRec.push_back(rightPage);

                while (true)
                {
                    leftIter->getCurrent(leftRecPtr2);
                    if (!leftComp() && !leftCompReverse())
                    {
                        if (!leftPage.append(leftRecPtr2))
                        {
                            MyDB_PageReaderWriter newPage(true, *(leftTable->getBufferMgr()));
                            leftPage = newPage;
                            leftVecRec.push_back(leftPage);
                            leftPage.append(leftRecPtr2);
                        }
                    }
                    else
                    {
                        break;
                    }

                    if (!leftIter->advance())
                    {
                        flag = true;
                        break;
                    }
                }

                while (true)
                {
                    rightIter->getCurrent(rightRecPtr);
                    if (equal()->toBool())
                    {
                        if (!rightPage.append(rightRecPtr))
                        {
                            MyDB_PageReaderWriter newPage(true, *(rightTable->getBufferMgr()));
                            rightPage = newPage;
                            rightVecRec.push_back(rightPage);
                            rightPage.append(rightRecPtr);
                        }
                    }
                    else
                    {

                        break;
                    }

                    if (!rightIter->advance())
                    {
                        flag = true;
                        break;
                    }
                }

                MyDB_RecordIteratorAltPtr leftVecRecIter;
                leftVecRecIter = getIteratorAlt(leftVecRec);

                while (leftVecRecIter->advance())
                {
                    leftVecRecIter->getCurrent(leftRecPtr);
                    MyDB_RecordIteratorAltPtr rightVecRecIter;
                    rightVecRecIter = getIteratorAlt(rightVecRec);

                    while (rightVecRecIter->advance())
                    {
                        rightVecRecIter->getCurrent(rightRecPtr);

                        if (finalPredicate()->toBool())
                        {
                            int i = 0;
                            for (auto &f : finalComputations)
                            {
                                outputRec->getAtt(i++)->set(f());
                            }
                            outputRec->recordContentHasChanged();
                            output->append(outputRec);
                        }
                    }
                }
            }
            if (flag)
            {
                break;
            }
        }
    }
}

#endif
