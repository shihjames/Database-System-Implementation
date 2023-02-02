# Database System Implementation

Author: James Shih

## Project Description

Building up the prototype of Buffer Manager, Buffer Handles, and Page.

1. Buffer Manager: Managing a pool of pages that are going to be used to buffer temporary data, and that will be used to buffer pages from database tables. This buffer manager will serve as the foundation that the rest of our database system.

2. Page Handle: The pages are accessed by the rest of the system via the handles. When a user requests the same page multiple times, the page is only added to the pool one time. However, multiple handles to the page are created, one for each request.

3. Page: There are two types of pages managed by this LRU buffer manager; pages that are associated with some position in an actual file/database table, and pages that are anonymous and are used as temporary storage by the rest of your system.

## Usage

```cpp
// create a buffer manager
MyDB_BufferManager myMgr (64, 16, "tempDSFSD");
MyDB_TablePtr table1 = make_shared <MyDB_Table> ("tempTable", "foobar");

// allocate a pinned page
MyDB_PageHandle pinnedPage = myMgr.getPinnedPage (table1, 0);
char *bytes = (char *) pinnedPage->getBytes ();
writeNums (bytes, 64, 0);
pinnedPage->wroteBytes ();
```
