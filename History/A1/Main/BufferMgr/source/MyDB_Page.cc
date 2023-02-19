
#ifndef PAGE_C
#define PAGE_C

#include <iostream>
#include "../headers/MyDB_BufferManager.h"
#include "../headers/MyDB_Page.h"
#include "../../Catalog/headers/MyDB_Table.h"

// access the raw byte of this page
void *MyDB_Page ::getBytes()
{
    bufferMgr.updateBuffer(*this);
    return bytePtr;
}

// change the status of isDirty
void MyDB_Page ::wroteBytes()
{
    this->isDirty = true;
}

// increase or decrease the value of refCount

// -1: set -1 0: cnt-- , 1: cnt ++
void MyDB_Page ::setRefCount(int val)
{
    if (val < 0)
    {
        this->refCount = -1;
    }
    else
    {
        if (val == 0)
        {
            this->refCount--;
            if (this->refCount == 0)
            {
                // cout << "Destruct " << this->pageID << endl;
                this->bufferMgr.handlePage(*this);
            }
        }
        else
        {
            this->refCount++;
        }
    }
}

// set anonymous
void MyDB_Page ::setAnonymous(bool anonyStatus)
{
    this->isAnony = anonyStatus;
}

// get anonymous
bool MyDB_Page ::getAnonymous()
{
    return this->isAnony;
}

// set dirty
void MyDB_Page ::setDirty(bool dirtyStatus)
{
    this->isDirty = dirtyStatus;
}

// get dirty
bool MyDB_Page ::getDirty()
{
    return this->isDirty;
}

// get table
MyDB_TablePtr MyDB_Page ::getTable()
{
    return this->whichTable;
}

// get pinned status
bool MyDB_Page ::getPinned()
{
    return this->isPinned;
}

void MyDB_Page ::setPinned(bool pinStatus)
{
    this->isPinned = pinStatus;
}

void MyDB_Page ::setNull()
{
    this->bytePtr = nullptr;
}

void MyDB_Page ::setIsSet(bool setStatus)
{
    this->isSet = setStatus;
}

bool MyDB_Page ::getIsSet()
{
    return this->isSet;
}

long MyDB_Page ::getPageID()
{
    return this->pageID;
}

int MyDB_Page ::getRefCount()
{
    return this->refCount;
}

// build a page
MyDB_Page ::MyDB_Page(MyDB_TablePtr currTable, MyDB_BufferManager &currBufferMgr, long ID, bool pinStatus, bool anonyStatus, bool setStatus)
    : pageID(ID), isAnony(anonyStatus), isPinned(pinStatus), isSet(setStatus), whichTable(currTable), bufferMgr(currBufferMgr)
{
    refCount = 0;
    isDirty = false;
    bytePtr = nullptr;
}

// destruct the page when no reference to this page
MyDB_Page ::~MyDB_Page()
{
    // free(bytePtr);
    // bytePtr = nullptr;
}

#endif
