#ifndef BUFFER_MGR_C
#define BUFFER_MGR_C

#include <iostream>
#include "../headers/MyDB_BufferManager.h"

using namespace std;

MyDB_PageHandle MyDB_BufferManager ::getPage(MyDB_TablePtr table, long pageID)
{

	// if table is a nullptr, meaning that table is not exists
	if (table == nullptr)
	{
		// print out error message and terminate the program
		cout << "There is no such table exists!" << endl;
		exit(1);
	}

	// create a pair to figure out whether this is an existing page
	pair<MyDB_TablePtr, long> currentPage = make_pair(table, pageID);

	// currentPage not exist in pageMap
	if (pageMap.count(currentPage) == 0)
	{
		// construct a new MyDB_Page object
		MyDB_Page *newPage = new MyDB_Page(table, *this, pageID, false, false, true);
		// add it in to pageMap
		pageMap[currentPage] = newPage;
		// return the shared pointer of newPage
		return make_shared<MyDB_PageHandleBase>(newPage);
	}
	// currentPage exists in pageMap
	else
	{
		// simply return the shared pointer of currentPage
		return make_shared<MyDB_PageHandleBase>(pageMap[currentPage]);
	}
}

MyDB_PageHandle MyDB_BufferManager ::getPage()
{
	// get a anonymous page ID
	long anonyPageID = this->getAnonyPageID();
	// create a pair to figure out whether this is an existing page
	pair<MyDB_TablePtr, long> currentPage = make_pair(nullptr, anonyPageID);

	// currentPage not exist in pageMap
	if (pageMap.count(currentPage) == 0)
	{
		// construct a new MyDB_Page object for each anonymous page
		MyDB_Page *newPage = new MyDB_Page(nullptr, *this, anonyPageID, false, true, true);
		// add 1 to anonymous page ID
		setAnonyPageID();
		// add it in to pageMap
		pageMap[currentPage] = newPage;
		// return the shared pointer of newPage
		return make_shared<MyDB_PageHandleBase>(newPage);
	}
	// currentPage exist in pageMap
	else
	{
		// simply return the shared pointer of currentPage
		return make_shared<MyDB_PageHandleBase>(pageMap[currentPage]);
	}
}

/*
(1) Check the page is in the buffer, if no go to (2), else go to (3)
(2) The page is not in the buffer, create a new Page object, go to (4)
(3) The page is already in the buffer, kickout from the LRU (recentAccessed), go to (4)
(4) Set the page as pinned
*/
MyDB_PageHandle MyDB_BufferManager ::getPinnedPage(MyDB_TablePtr table, long pageID)
{
	// if table is a nullptr, meaning that table is not exists
	if (table == nullptr)
	{
		cout << "There is no such table exists!" << endl;
		exit(1);
	}

	// create a pair to figure out whether this is an existing page
	pair<MyDB_TablePtr, long> currentPage = make_pair(table, pageID);
	MyDB_Page *thisPage;

	// currentPage not exist in pageMap
	if (pageMap.count(currentPage) == 0)
	{
		// construct a new page object
		thisPage = new MyDB_Page(table, *this, pageID, true, false, true);
		// add it in to pageMap
		pageMap[currentPage] = thisPage;
	}
	// currentPage exist in pageMap
	else
	{
		thisPage = pageMap[currentPage];
		// MyDB_PageHandle target = make_shared<MyDB_PageHandleBase>(thisPage);
		if (recentAccessed.find(currentPage) != recentAccessed.end())
		{
			MyDB_Page *delPage;
			delPage = recentAccessed[currentPage];
			// cout << "deleteeeee" << delPage)->getPageID() << endl;
			delete delPage;
			recentAccessed.erase(currentPage);
		}
	}
	// thisPage exist but not in buffer
	if ((*thisPage).bytePtr == nullptr)
	{
		// buffer has no sufficient space, execute kickout
		if (availableBuffer.size() == 0)
		{
			// kickout page
			if (kickout())
			{
				// (1) Load currentPage into buffer
				thisPage->bytePtr = availableBuffer[availableBuffer.size() - 1];
				availableBuffer.pop_back();
				// problem, not sure for the existance of this line
				pageMap[currentPage] = thisPage;
				// (2) set pinned true
				thisPage->setPinned(true);
			}
			// fail to kick out
			else
			{
				cout << "ERROR: No space to pin a page" << endl;
				return nullptr;
			}
		}
		// buffer already has sufficient space
		else
		{
			thisPage->bytePtr = availableBuffer[availableBuffer.size() - 1];
			// cout << "pop back availablebuffer" << endl;
			availableBuffer.pop_back();
			// problem, not sure for the existance of this line
			pageMap[currentPage] = thisPage;
			// set pinned true
			thisPage->setPinned(true);
		}
	}
	// thisPage exist and already in buffer, no need to change buffer, just setpinned
	else
	{
		thisPage->setPinned(true);
	}

	return make_shared<MyDB_PageHandleBase>(thisPage);
}

MyDB_PageHandle MyDB_BufferManager ::getPinnedPage()
{
	// buffer has no sufficient space, execute kickout
	if (availableBuffer.size() == 0)
	{
		if (!kickout())
		{
			cout << "ERROR: No space to pin a page" << endl;
			return nullptr;
		}
	}
	// get the anonymous page
	MyDB_PageHandle thisPage = getPage();
	// load currentPage into buffer
	MyDB_Page *currentPage = thisPage->getPagePtr();
	currentPage->bytePtr = availableBuffer[availableBuffer.size() - 1];
	availableBuffer.pop_back();
	// set pinned true
	currentPage->setPinned(true);
	return thisPage;
}

void MyDB_BufferManager ::unpin(MyDB_PageHandle unpinMe)
{
	MyDB_Page *currentPage = unpinMe->getPagePtr();
	pair<MyDB_TablePtr, long> findMe = make_pair((*currentPage).whichTable, (*currentPage).pageID);
	// unpin the page
	currentPage->isPinned = false;
	// set isSet to false
	currentPage->isSet = false;
	// add to recentAccessed
	recentAccessed[findMe] = currentPage;
}

// Use to handle buffer is full scenario, clock algorithm !!!
bool MyDB_BufferManager ::kickout()
{
	// availabelBuffer is full and noone can kick out(since everyone is pinned)
	if (recentAccessed.size() == 0)
	{
		return false;
	}

	// iterator of recentAccessed
	map<pair<MyDB_TablePtr, long>, MyDB_Page *>::iterator iter = armPos;
	// MyDB_PageHandle phb;
	MyDB_Page *currentPage;

	while (true)
	{
		if (iter == recentAccessed.end())
		{
			iter = recentAccessed.begin();
		}
		else
		{
			currentPage = recentAccessed[iter->first];
			// page is set -> unset the page
			if (currentPage->getIsSet())
			{
				currentPage->setIsSet(false);
				iter++;
			}
			// found unset page
			else
			{
				// update armPos then break the loop
				// check if the page is dirty
				if (currentPage->isDirty == true)
				{
					// if dirty, write back to disk
					int fd;
					if (currentPage->getAnonymous())
					{
						fd = open(tempFile.c_str(), O_CREAT | O_RDWR, 0666);
					}
					else
					{
						fd = open(currentPage->getTable()->getStorageLoc().c_str(), O_CREAT | O_RDWR, 0666);
					}
					lseek(fd, currentPage->pageID * pageSize, SEEK_SET);
					write(fd, currentPage->bytePtr, pageSize);
					// set dirty to false
					currentPage->setDirty(false);
					// remember to close it
					close(fd);
				}
				// delete it from recentAccessed
				armPos = recentAccessed.erase(iter);
				// add memory space to availableBuffer
				availableBuffer.push_back(currentPage->bytePtr);
				// set byteptr to nullptr
				currentPage->setNull();
				break;
			}
		}
	}
	return true;
}

void MyDB_BufferManager ::updateBuffer(MyDB_Page &toUpdate)
{
	pair<MyDB_TablePtr, long> findMe = make_pair(toUpdate.whichTable, toUpdate.pageID);
	MyDB_Page *updateMe = pageMap[findMe];

	// check if the page is in recentAccessed
	if (recentAccessed.find(findMe) != recentAccessed.end())
	{
		(*updateMe).setIsSet(true);
	}

	else
	{
		// filter unpinned page
		if (!((*updateMe).getPinned()))
		{
			if (this->availableBuffer.size() == 0)
			{
				// sufficient memory to insert to recentAccessed
				if (kickout())
				{
					// set isSet to true
					(*updateMe).setIsSet(true);
					// get RAM for the page
					(*updateMe).bytePtr = availableBuffer[availableBuffer.size() - 1];
					availableBuffer.pop_back();
					// read the bytes
					int fd;
					if ((*updateMe).getTable() == nullptr)
					{
						fd = open(tempFile.c_str(), O_CREAT | O_RDWR, 0666);
					}
					// this is a non-anonymous page
					else
					{
						fd = open((*updateMe).getTable()->getStorageLoc().c_str(), O_CREAT | O_RDWR, 0666);
					}
					lseek(fd, (*updateMe).pageID * this->pageSize, SEEK_SET);
					read(fd, (*updateMe).bytePtr, this->pageSize);
					// add the page handler to recentAccessed
					recentAccessed[findMe] = updateMe;
					// remember to close it
					close(fd);
				}
				// no more space
				else
				{
					cout << "No more available space" << endl;
					exit(1);
				}
			}
			else
			{
				// set isSet to true
				(*updateMe).setIsSet(true);
				// get RAM for the page
				(*updateMe).bytePtr = availableBuffer[availableBuffer.size() - 1];
				availableBuffer.pop_back();
				// read the bytes
				int fd;
				if ((*updateMe).getTable() == nullptr)
				{
					fd = open(tempFile.c_str(), O_CREAT | O_RDWR, 0666);
				}
				// this is a non-anonymous page
				else
				{
					fd = open((*updateMe).getTable()->getStorageLoc().c_str(), O_CREAT | O_RDWR, 0666);
				}

				// cout << "pageID " << (*updateMe).pageID <<" pagesize  " << (*updateMe).pageID * this->pageSize<< endl ;
				lseek(fd, (*updateMe).pageID * this->pageSize, SEEK_SET);
				read(fd, (*updateMe).bytePtr, this->pageSize);
				// add the page handler to recentAccessed
				recentAccessed[findMe] = updateMe;
				close(fd);
			}
		}
		// non-anonymous page
		else
		{
			if ((*updateMe).getTable() != nullptr)
			{
				// cout << "else bufferptr " << (*updateMe).bytePtr << " pageID: " << (*updateMe).getPageID() << " table: " << (*updateMe).getTable() << endl;
				int fd;
				if ((*updateMe).getTable() == nullptr)
				{
					fd = open(tempFile.c_str(), O_CREAT | O_RDWR, 0666);
				}
				// this is a non-anonymous page
				else
				{
					fd = open((*updateMe).getTable()->getStorageLoc().c_str(), O_CREAT | O_RDWR, 0666);
				}
				lseek(fd, (*updateMe).pageID * this->pageSize, SEEK_SET);
				read(fd, (*updateMe).bytePtr, this->pageSize);
				close(fd);
			}
		}
	}
}

void MyDB_BufferManager ::handlePage(MyDB_Page &handleMe)
{
	// find such page in pageMap
	handleMe.setRefCount(-1);

	pair<MyDB_TablePtr, long> currentPage = make_pair(handleMe.getTable(), handleMe.getPageID());
	if (pageMap.count(currentPage) != 0)
	{
		MyDB_Page *myPtr = pageMap[currentPage];
		MyDB_PageHandle temp = make_shared<MyDB_PageHandleBase>(myPtr);
		if (handleMe.bytePtr != nullptr && recentAccessed.count(currentPage) == 0)
		{
			myPtr->setRefCount(1);
			unpin(temp);
			return;
		}

		if (recentAccessed.count(currentPage) != 0)
		{
			recentAccessed.erase(currentPage);
		}
	}

	// if dirty
	if (handleMe.getDirty())
	{
		// this is an anonymous page
		int fd;
		if (handleMe.getTable() == nullptr)
		{
			fd = open(tempFile.c_str(), O_CREAT | O_RDWR, 0666);
		}
		// this is a non-anonymous page
		else
		{
			fd = open(handleMe.whichTable->getStorageLoc().c_str(), O_CREAT | O_RDWR, 0666);
		}
		// write back to disk
		lseek(fd, handleMe.pageID * this->pageSize, SEEK_SET);
		write(fd, handleMe.bytePtr, this->pageSize);
		handleMe.setDirty(false);
		// remember to close it
		close(fd);
	}

	// release memory to buffer
	if (handleMe.bytePtr != nullptr)
	{
		availableBuffer.push_back(handleMe.bytePtr);
		handleMe.bytePtr = nullptr;
	}

	auto page = pageMap.find(currentPage);
	pageMap.erase(page);
	delete page->second;
}

MyDB_BufferManager ::MyDB_BufferManager(size_t page_size, size_t num_pages, string temp_file)
{
	pageSize = page_size;
	numPages = num_pages;
	tempFile = temp_file;

	for (size_t page = 0; page < numPages; ++page)
	{
		availableBuffer.push_back(malloc(pageSize));
	}
	armPos = recentAccessed.end();
	anonyPageID = 0;
}

MyDB_BufferManager ::~MyDB_BufferManager()
{
	for (size_t page = 0; page < numPages; ++page)
	{
		free(availableBuffer[page]);
	}
	// unlike tempFile
	unlink(tempFile.c_str());
}

size_t MyDB_BufferManager ::getPageSize()
{
	return pageSize;
}

size_t MyDB_BufferManager ::getNumPages()
{
	return numPages;
}

long MyDB_BufferManager ::getAnonyPageID()
{
	return anonyPageID;
}

void MyDB_BufferManager ::setAnonyPageID()
{
	anonyPageID++;
}

vector<void *> MyDB_BufferManager ::getAvailablebuffer()
{
	return this->availableBuffer;
}

map<pair<MyDB_TablePtr, long>, MyDB_Page *> MyDB_BufferManager ::getRecentAccessed()
{
	return this->recentAccessed;
}

#endif