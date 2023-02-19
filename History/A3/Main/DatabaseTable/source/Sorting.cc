
#ifndef SORT_C
#define SORT_C

#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableRecIterator.h"
#include "MyDB_TableRecIteratorAlt.h"
#include "MyDB_TableReaderWriter.h"
#include "RecIterComparator.h"
#include "Sorting.h"
#include <queue>

using namespace std;

void mergeIntoFile(MyDB_TableReaderWriter &sortIntoMe, vector<MyDB_RecordIteratorAltPtr> &mergeUs, function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs)
{
	RecIterComparator comp(comparator, lhs, rhs);
	priority_queue<MyDB_RecordIteratorAltPtr, vector<MyDB_RecordIteratorAltPtr>, RecIterComparator> pq(comp);

	// push all records into priority queue
	for (auto recIterPtr : mergeUs)
	{
		pq.push(recIterPtr);
	}

	while (!pq.empty())
	{
		MyDB_RecordPtr curRec = sortIntoMe.getEmptyRecord();
		MyDB_RecordIteratorAltPtr temp = pq.top();
		temp->getCurrent(curRec);
		pq.pop();

		if (temp->advance())
		{
			pq.push(temp);
		}
		sortIntoMe.append(curRec);
	}
}

vector<MyDB_PageReaderWriter> mergeIntoList(MyDB_BufferManagerPtr myBufferMgr, MyDB_RecordIteratorAltPtr leftRecIterPtr, MyDB_RecordIteratorAltPtr rightRecIterPtr, function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs)
{

	vector<MyDB_PageReaderWriter> sortedPageList;
	MyDB_PageReaderWriter pageRW(*myBufferMgr);

	// record iterator and record pointer for remaining records
	MyDB_RecordIteratorAltPtr recIterPtr;
	MyDB_RecordPtr recPtr;

	// add current page reader writer to list
	sortedPageList.push_back(pageRW);

	while (true)
	{
		// load the current record to lhs and rhs
		leftRecIterPtr->getCurrent(lhs);
		rightRecIterPtr->getCurrent(rhs);

		// begin comparison
		if (comparator())
		{
			// lhs is smaller, try to append it to page reader writer
			if (!pageRW.append(lhs))
			{
				// insufficient space, create a new page reader writer
				pageRW = MyDB_PageReaderWriter(*myBufferMgr);
				sortedPageList.push_back(pageRW);
				// append it to the new page reader writer
				pageRW.append(lhs);
			}
			// check if there are remaining records
			if (!leftRecIterPtr->advance())
			{
				// leftRecIterPtr have traverse to the end
				// set record iterator and record pointer for remaining records (right)
				recPtr = rhs;
				recIterPtr = rightRecIterPtr;
				break;
			}
			leftRecIterPtr->getCurrent(lhs);
		}
		else
		{
			if (!pageRW.append(rhs))
			{
				pageRW = MyDB_PageReaderWriter(*myBufferMgr);
				sortedPageList.push_back(pageRW);
				pageRW.append(rhs);
			}
			if (!rightRecIterPtr->advance())
			{
				recPtr = lhs;
				recIterPtr = leftRecIterPtr;
				break;
			}
			rightRecIterPtr->getCurrent(rhs);
		}
	}

	// complete adding remianing record to page reader writer
	do
	{
		recIterPtr->getCurrent(recPtr);

		if (!pageRW.append(recPtr))
		{
			pageRW = MyDB_PageReaderWriter(*myBufferMgr);
			sortedPageList.push_back(pageRW);
			pageRW.append(recPtr);
		}
	} while (recIterPtr->advance());

	return sortedPageList;
}

vector<MyDB_PageReaderWriter> mergeSort(vector<MyDB_PageReaderWriter> &runPages, MyDB_BufferManagerPtr myBufferMgr, function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs, int lower, int upper)
{
	// list with only one page, simply return it
	if (lower >= upper)
	{
		return vector<MyDB_PageReaderWriter>(runPages.begin() + lower, runPages.begin() + lower + 1);
	}
	// recursively sort the pages with sortIntoList()
	else
	{
		int mid = lower + (upper - lower) / 2;
		vector<MyDB_PageReaderWriter> vector1 = mergeSort(runPages, myBufferMgr, comparator, lhs, rhs, lower, mid);
		vector<MyDB_PageReaderWriter> vector2 = mergeSort(runPages, myBufferMgr, comparator, lhs, rhs, mid + 1, upper);
		MyDB_RecordIteratorAltPtr leftRecIterPtr = getIteratorAlt(vector1);
		MyDB_RecordIteratorAltPtr rightRecIterPtr = getIteratorAlt(vector2);
		vector<MyDB_PageReaderWriter> sortedPageList = mergeIntoList(myBufferMgr, leftRecIterPtr, rightRecIterPtr, comparator, lhs, rhs);
		return sortedPageList;
	}
}

void sort(int runSize, MyDB_TableReaderWriter &sortMe, MyDB_TableReaderWriter &sortIntoMe, function<bool()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs)
{
	// get number of pages and create a counter for read pages
	int numPages = sortMe.getNumPages(), pageIndex = 0;

	// create vectors to manage sorting
	vector<vector<MyDB_PageReaderWriter>> allRuns;

	// load all page
	while (pageIndex < numPages)
	{
		vector<MyDB_PageReaderWriter> runPages;
		// load a run of pages into RAM
		for (int i = 0; i < runSize; i++)
		{
			runPages.push_back(*(sortMe[pageIndex].sort(comparator, lhs, rhs)));
			pageIndex++;
			if (pageIndex >= numPages)
			{
				break;
			}
		}
		runPages = mergeSort(runPages, sortMe.getBufferMgr(), comparator, lhs, rhs, 0, runPages.size() - 1);
		// append the sorted run to allPages
		allRuns.push_back(runPages);
	}

	// mergeIntoFile
	vector<MyDB_RecordIteratorAltPtr> mergeUs;
	for (auto run : allRuns)
	{
		mergeUs.push_back(getIteratorAlt(run));
	}

	mergeIntoFile(sortIntoMe, mergeUs, comparator, lhs, rhs);
}

#endif
