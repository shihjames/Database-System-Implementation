#ifndef AGG_CC
#define AGG_CC

#include "MyDB_Record.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"
#include "Aggregate.h"
#include <unordered_map>
#include "MyDB_Schema.h"

using namespace std;

Aggregate ::Aggregate(MyDB_TableReaderWriterPtr input, MyDB_TableReaderWriterPtr output,
                      vector<pair<MyDB_AggType, string>> aggsToCompute,
                      vector<string> groupings, string selectionPredicate)
{
    this->input = input;
    this->output = output;
    this->aggsToCompute = aggsToCompute;
    this->groupings = groupings;
    this->selectionPredicate = selectionPredicate;
}

void Aggregate ::run()
{
    if (this->output->getTable()->getSchema()->getAtts().size() != aggsToCompute.size() + groupings.size())
    {
        cout << "[ERROR]| Size of output should be equal to the sum of size of aggsToCompute and size of groupings.\n";
        return;
    }

    unordered_map<size_t, void *> myHash;
    unordered_map<size_t, size_t> cntMap;
    unordered_map<size_t, vector<double>> sumMap;
    unordered_map<size_t, vector<double>> avgMap;

    MyDB_SchemaPtr mySchemaOut = make_shared<MyDB_Schema>();
    for (auto p : output->getTable()->getSchema()->getAtts())
    {
        mySchemaOut->appendAtt(p);
    }

    // get an empty record
    MyDB_RecordPtr inputRec = input->getEmptyRecord();
    // create output record using outout table schema
    MyDB_RecordPtr outputRec = make_shared<MyDB_Record>(mySchemaOut);

    vector<func> groupFunc;
    for (auto g : groupings)
    {
        groupFunc.push_back(inputRec->compileComputation(g));
    }

    vector<pair<MyDB_AggType, func>> aggFunc;
    for (auto agg : aggsToCompute)
    {
        aggFunc.push_back(make_pair(agg.first, inputRec->compileComputation(agg.second)));
    }

    func pred = inputRec->compileComputation(selectionPredicate);

    // get all of the pages
    vector<MyDB_PageReaderWriter> allData;
    for (int i = 0; i < input->getNumPages(); i++)
    {
        MyDB_PageReaderWriter temp = (*input)[i];
        if (temp.getType() == MyDB_PageType ::RegularPage)
        {
            allData.push_back(temp);
        }
    }

    // get the iterator for all records in allData
    MyDB_RecordIteratorAltPtr myIter = getIteratorAlt(allData);

    int lastPage = 0;
    while (myIter->advance())
    {
        // hash the current record
        myIter->getCurrent(inputRec);

        // see if it is accepted by the predicate
        if (!pred()->toBool())
        {
            continue;
        }

        // compute its hash
        size_t hashVal = 0;
        for (auto f : groupFunc)
        {
            hashVal ^= f()->hash();
        }
        // if nothing found in this hash, create one
        if (myHash.count(hashVal) == 0)
        {
            cntMap[hashVal] = 1;

            // run all of the grouping function
            int i = 0;
            for (auto f : groupFunc)
            {
                outputRec->getAtt(i++)->set(f());
            }
            // run all aggregation function
            int aggIndex = groupFunc.size();
            for (auto agg : aggFunc)
            {
                switch (agg.first)
                {
                case MyDB_AggType::cnt:
                {
                    MyDB_IntAttValPtr att = make_shared<MyDB_IntAttVal>();
                    att->set(1);
                    outputRec->getAtt(aggIndex)->set(att);
                    break;
                }
                case MyDB_AggType::sum:
                {
                    outputRec->getAtt(aggIndex)->set(agg.second());
                    sumMap[hashVal].push_back(outputRec->getAtt(aggIndex)->toDouble());
                    break;
                }
                case MyDB_AggType::avg:
                {
                    outputRec->getAtt(aggIndex)->set(agg.second());
                    avgMap[hashVal].push_back(outputRec->getAtt(aggIndex)->toDouble());
                    break;
                }
                }
                aggIndex++;
            }

            outputRec->recordContentHasChanged();

            MyDB_PageReaderWriter tempPage = output->getPinned(lastPage);
            void *loc = tempPage.appendAndReturnLocation(outputRec);

            // myHash[hashVal] = loc;

            // if no page has found, create one for the incoming record.
            if (loc == nullptr)
            {
                tempPage = output->getPinned(lastPage++);
                myHash[hashVal] = tempPage.appendAndReturnLocation(outputRec);
            }

            myHash[hashVal] = loc;
        }
        else
        {
            outputRec->fromBinary(myHash[hashVal]);
            cntMap[hashVal] += 1;

            int aggIndex = groupFunc.size();
            int sumCnt = 0;
            int avgCnt = 0;
            for (auto agg : aggFunc)
            {
                switch (agg.first)
                {
                case MyDB_AggType::cnt:
                {
                    MyDB_IntAttValPtr att = make_shared<MyDB_IntAttVal>();
                    att->set(cntMap[hashVal]);
                    outputRec->getAtt(aggIndex)->set(att);
                    break;
                }
                case MyDB_AggType::sum:
                {
                    outputRec->getAtt(aggIndex)->set(agg.second());
                    sumMap[hashVal][sumCnt] += outputRec->getAtt(aggIndex)->toDouble();
                    MyDB_IntAttValPtr att = make_shared<MyDB_IntAttVal>();
                    att->set(sumMap[hashVal][sumCnt]);
                    outputRec->getAtt(aggIndex)->set(att);
                    sumCnt++;
                    break;
                }
                case MyDB_AggType::avg:
                {
                    outputRec->getAtt(aggIndex)->set(agg.second());
                    avgMap[hashVal][avgCnt] = (avgMap[hashVal][avgCnt] * (cntMap[hashVal] - 1) + outputRec->getAtt(aggIndex)->toDouble()) / cntMap[hashVal];
                    MyDB_DoubleAttValPtr att = make_shared<MyDB_DoubleAttVal>();
                    att->set(avgMap[hashVal][avgCnt]);
                    outputRec->getAtt(aggIndex)->set(att);
                    avgCnt++;
                    break;
                }
                }
                aggIndex++;
            }

            outputRec->recordContentHasChanged();
            outputRec->toBinary(myHash[hashVal]);
            // output->append(outputRec);

            // MyDB_PageReaderWriter tempPage = output->getPinned(lastPage);
            // void *loc = tempPage.appendAndReturnLocation(outputRec);

            // // myHash[hashVal] = loc;

            // // if no page has found, create one for the incoming record.
            // if (loc == nullptr)
            // {
            //     tempPage = output->getPinned(lastPage++);
            //     myHash[hashVal] = tempPage.appendAndReturnLocation(outputRec);
            // }

            // myHash[hashVal] = loc;
        }
    }
}

#endif
