// File: BufferManager.h
#pragma once

class BufferManager {
public:
    BufferManager();
    // Fetch a page into buffer pool
    char *fetchPage(int pageId);
    // Pin/Unpin
    void pinPage(int pageId);
    void unpinPage(int pageId);
private:
    // TODO: implement LRU/Clock, page frames
};
