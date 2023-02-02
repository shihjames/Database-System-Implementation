
#ifndef PAGE_H
#define PAGE_H

#include <memory>
#include "../../Catalog/headers/MyDB_Table.h"

class MyDB_BufferManager;

using namespace std;
// typedef MyDB_Page *MyDB_PagePtr;

class MyDB_Page
{

public:
    // access the raw byte of this page
    void *getBytes();

    // change the status of isDirty
    void wroteBytes();

    // increase or decrease the value of refCount
    void setRefCount(int decrease);

    // build a page
    MyDB_Page(MyDB_TablePtr whichTable, MyDB_BufferManager &bufferMgr, long pageID, bool isPinned, bool isAnony, bool isSet);

    // destruct the page when no reference to this page
    ~MyDB_Page();

    // set anonymous
    void setAnonymous(bool anonyStatus);

    // get anonymous status
    bool getAnonymous();

    // set dirty
    void setDirty(bool dirtyStatus);

    // get dirty
    bool getDirty();

    // get table
    MyDB_TablePtr getTable();

    // get pinned
    bool getPinned();

    // set pinned
    void setPinned(bool pinStatus);

    // set byte pointer to null
    void setNull();

    // set isSet
    void setIsSet(bool setStatus);

    // get isSet
    bool getIsSet();

    // get page ID
    long getPageID();

    // get reference count
    int getRefCount();

private:
    // friend classes
    friend class MyDB_BufferManager;

    // pointer to the raw bytes of the page
    void *bytePtr;

    // ID belongs to a certain page
    long pageID;

    // used to count the number of PHB
    int refCount;

    // check if the page is an anonymous page
    bool isAnony;

    // check if the page is a pinned page
    bool isPinned;

    // check if the page has been modified
    bool isDirty;

    // check if the page is accessed recently.
    bool isSet;

    // pointer to the table where the page belongs to
    MyDB_TablePtr whichTable;

    // pointer to the buffer manager
    MyDB_BufferManager &bufferMgr;
};

#endif
