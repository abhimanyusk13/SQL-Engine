// File: BufferManager.h
#pragma once

#include <vector>
#include <unordered_map>
#include <cstddef>
#include "FileManager.h"

class BufferManager {
public:
    // Create a buffer pool of given size (number of pages)
    explicit BufferManager(FileManager &fm, std::size_t poolSize = 128);
    ~BufferManager();

    // Fetches the specified page into the buffer pool (pin++). Returns pointer to page data.
    char *fetchPage(int fileId, int pageId);
    // Pin/unpin allow managing multiple users of the same page
    void pinPage(int fileId, int pageId);
    void unpinPage(int fileId, int pageId);
    // Mark the page as dirty so it will be written back on eviction or flush
    void markDirty(int fileId, int pageId);

    // Explicitly write a dirty page back to disk
    void flushPage(int fileId, int pageId);
    // Flush all dirty pages
    void flushAllPages();

private:
    // Identifier for a page in a file
    struct PageId {
        int fileId;
        int pageId;
        bool operator==(PageId const &o) const noexcept {
            return fileId == o.fileId && pageId == o.pageId;
        }
    };
    struct PageIdHash {
        std::size_t operator()(PageId const &p) const noexcept {
            return std::hash<unsigned long long>()(
                (static_cast<unsigned long long>(p.fileId) << 32) |
                static_cast<unsigned long long>(p.pageId)
            );
        }
    };

    struct Frame {
        bool isValid = false;
        bool isDirty = false;
        int pinCount = 0;
        PageId pid = {-1, -1};
        char *data = nullptr;
    };

    FileManager &fm_;
    std::size_t poolSize_;
    std::vector<Frame> frames_;
    std::unordered_map<PageId, std::size_t, PageIdHash> pageTable_;
    std::size_t clockHand_ = 0;

    // Find a frame to evict using CLOCK algorithm
    std::size_t selectVictimFrame();
};