
#ifndef PAGE_HANDLE_C
#define PAGE_HANDLE_C

#include <iostream>
#include <memory>
#include "../headers/MyDB_Page.h"
#include "../headers/MyDB_PageHandle.h"

void *MyDB_PageHandleBase ::getBytes()
{
    return (char *)(currentPage->getBytes());
}

void MyDB_PageHandleBase ::wroteBytes()
{
    this->currentPage->wroteBytes();
}

MyDB_Page *MyDB_PageHandleBase ::getPagePtr()
{
    return this->currentPage;
}

MyDB_BufferManager &MyDB_PageHandleBase ::getParent()
{
    return currentPage->getParent();
}

MyDB_PageHandleBase ::MyDB_PageHandleBase(MyDB_Page *newPage)
{
    this->currentPage = newPage;
    this->currentPage->setRefCount(1);
}

MyDB_PageHandleBase ::~MyDB_PageHandleBase()
{
    currentPage->setRefCount(0);
}

#endif
