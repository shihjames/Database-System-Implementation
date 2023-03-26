
#ifndef BPLUS_C
#define BPLUS_C

#include "MyDB_INRecord.h"
#include "MyDB_BPlusTreeReaderWriter.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_PageListIteratorSelfSortingAlt.h"
#include "RecordComparator.h"

MyDB_BPlusTreeReaderWriter ::MyDB_BPlusTreeReaderWriter(string orderOnAttName, MyDB_TablePtr forMe, MyDB_BufferManagerPtr myBuffer) : MyDB_TableReaderWriter(forMe, myBuffer)
{

	// find the ordering attribute
	auto res = forMe->getSchema()->getAttByName(orderOnAttName);

	// remember information about the ordering attribute
	// attribute type pointer
	orderingAttType = res.second;
	// index of attribute
	whichAttIsOrdering = res.first;

	// and the root location
	rootLocation = getTable()->getRootLocation();
}

MyDB_RecordIteratorAltPtr MyDB_BPlusTreeReaderWriter ::getSortedRangeIteratorAlt(MyDB_AttValPtr low, MyDB_AttValPtr high)
{

	vector<MyDB_PageReaderWriter> pageList;

	this->discoverPages(this->rootLocation, pageList, low, high);

	MyDB_RecordPtr lhs = this->getEmptyRecord();
	MyDB_RecordPtr rhs = this->getEmptyRecord();
	MyDB_RecordPtr rec = this->getEmptyRecord();

	MyDB_INRecordPtr lhsIN = this->getINRecord();
	MyDB_INRecordPtr rhsIN = this->getINRecord();

	lhsIN->setKey(low);
	rhsIN->setKey(high);

	function<bool()> comparator = this->buildComparator(lhs, rhs);
	function<bool()> lowComparator = this->buildComparator(rec, lhsIN);
	function<bool()> highComparator = this->buildComparator(rhsIN, rec);

	return make_shared<MyDB_PageListIteratorSelfSortingAlt>(pageList, lhs, rhs, comparator, rec, lowComparator, highComparator, true);
}

MyDB_RecordIteratorAltPtr MyDB_BPlusTreeReaderWriter ::getRangeIteratorAlt(MyDB_AttValPtr low, MyDB_AttValPtr high)
{

	vector<MyDB_PageReaderWriter> pageList;

	this->discoverPages(this->rootLocation, pageList, low, high);

	MyDB_RecordPtr lhs = this->getEmptyRecord();
	MyDB_RecordPtr rhs = this->getEmptyRecord();
	MyDB_RecordPtr rec = this->getEmptyRecord();

	MyDB_INRecordPtr lhsIN = this->getINRecord();
	MyDB_INRecordPtr rhsIN = this->getINRecord();

	lhsIN->setKey(low);
	rhsIN->setKey(high);

	function<bool()> comparator = this->buildComparator(lhs, rhs);
	function<bool()> lowComparator = this->buildComparator(rec, lhsIN);
	function<bool()> highComparator = this->buildComparator(rhsIN, rec);

	return make_shared<MyDB_PageListIteratorSelfSortingAlt>(pageList, lhs, rhs, comparator, rec, lowComparator, highComparator, false);
}

bool MyDB_BPlusTreeReaderWriter ::discoverPages(int whichPage, vector<MyDB_PageReaderWriter> &list,
												MyDB_AttValPtr low, MyDB_AttValPtr high)
{
	queue<int> q;
	q.push(whichPage);

	while (!q.empty())
	{
		MyDB_PageReaderWriter curPage = this->operator[](q.front());
		q.pop();

		if (curPage.getType() == MyDB_PageType ::RegularPage)
		{
			list.push_back(curPage);
		}
		else
		{
			MyDB_INRecordPtr lowIN = getINRecord();
			MyDB_INRecordPtr highIN = getINRecord();
			MyDB_INRecordPtr curIN = getINRecord();
			MyDB_RecordIteratorAltPtr iter = curPage.getIteratorAlt();

			lowIN->setKey(low);
			highIN->setKey(high);

			function<bool()> lComp = buildComparator(curIN, lowIN);
			function<bool()> hComp = buildComparator(highIN, curIN);

			while (iter->advance())
			{
				iter->getCurrent(curIN);
				if (lComp())
				{
					// curIN < lowIN
					continue;
				}

				q.push(curIN->getPtr());

				if (hComp())
				{
					// curIN > highIN
					break;
				}
			}
		}
	}

	// return if we found any page
	return !list.empty();
}

void MyDB_BPlusTreeReaderWriter ::append(MyDB_RecordPtr appendMe)
{

	// empty tree -> start with a root and a leaf
	if (rootLocation == -1)
	{
		// create a new root
		MyDB_PageReaderWriter newRoot = (*this)[++rootLocation];
		getTable()->setRootLocation(rootLocation);

		// create a new leaf
		MyDB_PageReaderWriter newLeaf = (*this)[1];
		// update last page
		getTable()->setLastPage(1);

		// create a internal node and add it to the directory page (root)
		MyDB_INRecordPtr INRec = getINRecord();
		// point to page 1
		INRec->setPtr(1);

		// add that internal node record in
		newRoot.clear();
		newRoot.append(INRec);
		newRoot.setType(MyDB_PageType ::DirectoryPage);

		// add the record to the new leaf
		newLeaf.clear();
		newLeaf.setType(MyDB_PageType ::RegularPage);
		newLeaf.append(appendMe);
	}
	else
	{
		// try to append the record to the tree
		MyDB_RecordPtr ret = append(rootLocation, appendMe);

		// see if the root split
		if (ret != nullptr)
		{

			// creat a new root
			getTable()->setLastPage(getTable()->lastPage() + 1);
			int updatedLoc = getTable()->lastPage();

			MyDB_PageReaderWriter newRoot = (*this)[updatedLoc];
			newRoot.clear();
			newRoot.setType(MyDB_PageType ::DirectoryPage);

			// create a internal node and pointed to the original root
			MyDB_INRecordPtr INRec = getINRecord();
			INRec->setPtr(rootLocation);

			// append new both internal nodes
			newRoot.append(ret);
			newRoot.append(INRec);

			// update the root location
			rootLocation = updatedLoc;
			getTable()->setRootLocation(rootLocation);
		}
	}
}

MyDB_RecordPtr MyDB_BPlusTreeReaderWriter ::split(MyDB_PageReaderWriter splitMe, MyDB_RecordPtr addMe)
{
	int lastPage = getTable()->lastPage() + 1;
	getTable()->setLastPage(lastPage);

	// create a new page for this index
	MyDB_PageReaderWriter newPage = (*this)[lastPage];
	newPage.clear();

	int temp = getTable()->lastPage() + 1;
	MyDB_PageReaderWriter tempPage = (*this)[temp];
	tempPage.clear();

	MyDB_RecordPtr lhs;
	MyDB_RecordPtr rhs;
	// for iteration
	MyDB_RecordPtr rec;

	bool isInternal = false;

	if (splitMe.getType() == MyDB_PageType ::RegularPage)
	{
		// handle regular pages case
		newPage.setType(MyDB_PageType::RegularPage);
		tempPage.setType(MyDB_PageType::RegularPage);
		lhs = getEmptyRecord();
		rhs = getEmptyRecord();
		rec = getEmptyRecord();
	}
	else
	{
		// handle internal pages case
		newPage.setType(MyDB_PageType::DirectoryPage);
		tempPage.setType(MyDB_PageType::DirectoryPage);
		lhs = getINRecord();
		rhs = getINRecord();
		rec = getINRecord();
		isInternal = true;
	}

	function<bool()> comparator = buildComparator(lhs, rhs);
	splitMe.sortInPlace(comparator, lhs, rhs);

	MyDB_RecordIteratorAltPtr recordIter = splitMe.getIteratorAlt();

	// cnt for record in the page
	int recCnt = 0;
	// MyDB_RecordPtr record = getINRecord();
	while (recordIter->advance())
	{
		recordIter->getCurrent(rec);
		recCnt++;
	}

	recordIter = splitMe.getIteratorAlt();
	// set the iterator back
	if (splitMe.getType() == MyDB_PageType ::RegularPage)
	{
		rec = getEmptyRecord();
	}
	else
	{
		rec = getINRecord();
	}

	bool flag = true;
	int cnt = 0;
	MyDB_INRecordPtr resPtr = getINRecord();
	resPtr->setPtr(lastPage);

	while (recordIter->advance())
	{
		recordIter->getCurrent(rec);
		if (cnt == ((recCnt / 2) - 1) && flag == true)
		{
			flag = false;
			resPtr->setKey(getKey(rec));
		}
		if (cnt <= ((recCnt / 2) - 1))
		{
			newPage.append(rec);
		}
		else
		{
			tempPage.append(rec);
		}
		cnt++;
	}
	function<bool()> compareSplit = buildComparator(addMe, resPtr);
	if (compareSplit())
	{
		newPage.append(addMe);
		newPage.sortInPlace(compareSplit, lhs, rhs);
	}
	else
	{
		tempPage.append(addMe);
		tempPage.sortInPlace(compareSplit, lhs, rhs);
	}

	// rec = getEmptyRecord();
	if (splitMe.getType() == MyDB_PageType ::RegularPage)
	{
		rec = getEmptyRecord();
	}
	else
	{
		rec = getINRecord();
	}
	// copy back to original page
	splitMe.clear();

	if (isInternal)
	{
		splitMe.setType(MyDB_PageType::DirectoryPage);
	}
	recordIter = tempPage.getIteratorAlt();

	while (recordIter->advance())
	{
		recordIter->getCurrent(rec);
		splitMe.append(rec);
	}

	// clear temppage we set for future use
	tempPage.clear();
	return resPtr;
}

MyDB_RecordPtr MyDB_BPlusTreeReaderWriter ::append(int whichPage, MyDB_RecordPtr appendMe)
{

	// target page to append the record to
	MyDB_PageReaderWriter target = (*this)[whichPage];

	// still a directory page
	// need to recursive find the target page (leaf node)
	if (target.getType() == MyDB_PageType ::DirectoryPage)
	{
		// create iterator for the current directory page
		MyDB_RecordIteratorAltPtr temp = target.getIteratorAlt();
		MyDB_INRecordPtr curIN = getINRecord();
		function<bool()> comparator = buildComparator(appendMe, curIN);
		while (temp->advance())
		{
			// see if the new key is less than the key in the directory record
			temp->getCurrent(curIN);
			if (comparator())
			{

				// recursively append
				MyDB_RecordPtr ret = append(curIN->getPtr(), appendMe);

				if (ret != nullptr)
				{
					// add the INRec to the directory page
					if (target.append(ret))
					{
						// successfully added the sort the page
						MyDB_INRecordPtr curIN = getINRecord();
						function<bool()> comparator = buildComparator(ret, curIN);
						target.sortInPlace(comparator, ret, curIN);
						return nullptr;
					}
					// current directory page is full, split the directory page
					return split(target, ret);
				}

				// no split occurs return nullptr
				return nullptr;
			}
		}
	}
	else
	{
		// find a leaf page, try to append the record
		if (target.append(appendMe))
		{
			// no split required
			return nullptr;
		}

		// split the page
		return split(target, appendMe);
	}

	return nullptr;
}

MyDB_INRecordPtr MyDB_BPlusTreeReaderWriter ::getINRecord()
{
	return make_shared<MyDB_INRecord>(orderingAttType->createAttMax());
}

MyDB_AttValPtr MyDB_BPlusTreeReaderWriter ::getKey(MyDB_RecordPtr fromMe)
{

	// in this case, got an IN record
	if (fromMe->getSchema() == nullptr)
		return fromMe->getAtt(0)->getCopy();

	// in this case, got a data record
	else
		return fromMe->getAtt(whichAttIsOrdering)->getCopy();
}

function<bool()> MyDB_BPlusTreeReaderWriter ::buildComparator(MyDB_RecordPtr lhs, MyDB_RecordPtr rhs)
{

	MyDB_AttValPtr lhAtt, rhAtt;

	// in this case, the LHS is an IN record
	if (lhs->getSchema() == nullptr)
	{
		lhAtt = lhs->getAtt(0);

		// here, it is a regular data record
	}
	else
	{
		lhAtt = lhs->getAtt(whichAttIsOrdering);
	}

	// in this case, the LHS is an IN record
	if (rhs->getSchema() == nullptr)
	{
		rhAtt = rhs->getAtt(0);

		// here, it is a regular data record
	}
	else
	{
		rhAtt = rhs->getAtt(whichAttIsOrdering);
	}

	// now, build the comparison lambda and return
	if (orderingAttType->promotableToInt())
	{
		return [lhAtt, rhAtt]
		{ return lhAtt->toInt() < rhAtt->toInt(); };
	}
	else if (orderingAttType->promotableToDouble())
	{
		return [lhAtt, rhAtt]
		{ return lhAtt->toDouble() < rhAtt->toDouble(); };
	}
	else if (orderingAttType->promotableToString())
	{
		return [lhAtt, rhAtt]
		{ return lhAtt->toString() < rhAtt->toString(); };
	}
	else
	{
		cout << "This is bad... cannot do anything with the >.\n";
		exit(1);
	}
}

#endif
