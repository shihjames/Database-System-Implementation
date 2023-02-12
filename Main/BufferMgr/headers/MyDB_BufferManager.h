
#ifndef BUFFER_MGR_H
#define BUFFER_MGR_H

#include "MyDB_PageHandle.h"
#include "MyDB_Page.h"
#include "../../Catalog/headers/MyDB_Table.h"

#include <fstream>
#include <vector>
#include <map>
#include <unordered_set>
#include <string>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <utility>

using namespace std;

class MyDB_BufferManager;
typedef shared_ptr<MyDB_BufferManager> MyDB_BufferManagerPtr;

class MyDB_BufferManager
{

public:
	// THESE METHODS MUST APPEAR AND THE PROTOTYPES CANNOT CHANGE!

	// gets the i^th page in the table whichTable... note that if the page
	// is currently being used (that is, the page is current buffered) a handle
	// to that already-buffered page should be returned
	MyDB_PageHandle getPage(MyDB_TablePtr whichTable, long i);

	// gets a temporary page that will no longer exist (1) after the buffer manager
	// has been destroyed, or (2) there are no more references to it anywhere in the
	// program.  Typically such a temporary page will be used as buffer memory.
	// since it is just a temp page, it is not associated with any particular
	// table
	MyDB_PageHandle getPage();

	// gets the i^th page in the table whichTable... the only difference
	// between this method and getPage (whicTable, i) is that the page will be
	// pinned in RAM; it cannot be written out to the file
	MyDB_PageHandle getPinnedPage(MyDB_TablePtr whichTable, long i);

	// gets a temporary page, like getPage (), except that this one is pinned
	MyDB_PageHandle getPinnedPage();

	// un-pins the specified page
	void unpin(MyDB_PageHandle unpinMe);

	// creates an LRU buffer manager... params are as follows:
	// 1) the size of each page is pageSize
	// 2) the number of pages managed by the buffer manager is numPages;
	// 3) temporary pages are written to the file tempFile
	MyDB_BufferManager(size_t pageSize, size_t numPages, string tempFile);

	// when the buffer manager is destroyed, all of the dirty pages need to be
	// written back to disk, any necessary data needs to be written to the catalog,
	// and any temporary files need to be deleted
	~MyDB_BufferManager();

	// get the page size
	size_t getPageSize();

	// get number of pages
	size_t getNumPages();

	// get current anonymous page ID
	long getAnonyPageID();

	// Add 1 to anonymous ID
	void setAnonyPageID();

	// get availableBuffer
	vector<void *> getAvailablebuffer();

	// get recentAccessed
	map<pair<MyDB_TablePtr, long>, MyDB_Page *> getRecentAccessed();

private:
	// friend classes:
	// in order to access accessed bit(isSet())
	friend class MyDB_Page;

	// fixed page size, given by the user
	size_t pageSize;

	// fixed number of pages, given by the user
	size_t numPages;

	// fixed temporary file name, given by the user
	string tempFile;

	// record anonymous pageID
	long anonyPageID;

	// map for all pages,{key, value} => {(table, pageID), page}
	map<pair<MyDB_TablePtr, long>, MyDB_Page *> pageMap;

	// create the available buffer memory in RAM
	vector<void *> availableBuffer;

	// store recent accessed page, when add one page then pop out one from availableBuffer
	map<pair<MyDB_TablePtr, long>, MyDB_Page *> recentAccessed;
	// unordered_set<MyDB_Page*> recentAccessed;

	// pointer for clock algo, check current pointing element
	map<pair<MyDB_TablePtr, long>, MyDB_Page *>::iterator armPos;

	// kickout page to release memory
	bool kickout();

	// update the buffer if accessed
	void updateBuffer(MyDB_Page &accessMe);

	// kill the page
	void handlePage(MyDB_Page &handleMe);
};

#endif
