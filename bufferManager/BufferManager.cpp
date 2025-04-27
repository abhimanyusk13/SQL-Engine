// File: BufferManager.cpp
#include "BufferManager.h"
#include <stdexcept>
#include <algorithm>

BufferManager::BufferManager(FileManager &fm, std::size_t poolSize)
    : fm_(fm), poolSize_(poolSize), frames_(poolSize) {
    // Allocate page buffers
    for (auto &f : frames_) {
        f.data = new char[FileManager::PAGE_SIZE];
    }
}

BufferManager::~BufferManager() {
    flushAllPages();
    for (auto &f : frames_) {
        delete[] f.data;
    }
}

char *BufferManager::fetchPage(int fileId, int pageId) {
    PageId pid{fileId, pageId};
    auto it = pageTable_.find(pid);
    if (it != pageTable_.end()) {
        // Already in pool
        Frame &f = frames_[it->second];
        f.pinCount++;
        return f.data;
    }

    // Need to load into pool
    std::size_t frameIdx = selectVictimFrame();
    Frame &victim = frames_[frameIdx];

    // If valid and dirty, write back
    if (victim.isValid) {
        if (victim.isDirty) {
            fm_.writePage(victim.pid.fileId, victim.pid.pageId, victim.data);
            victim.isDirty = false;
        }
        // Remove old mapping
        pageTable_.erase(victim.pid);
    }

    // Read new page
    fm_.readPage(fileId, pageId, victim.data);
    victim.isValid = true;
    victim.isDirty = false;
    victim.pinCount = 1;
    victim.pid = pid;
    pageTable_[pid] = frameIdx;
    return victim.data;
}

void BufferManager::pinPage(int fileId, int pageId) {
    PageId pid{fileId, pageId};
    auto it = pageTable_.find(pid);
    if (it == pageTable_.end()) throw std::runtime_error("Page not in buffer pool");
    frames_[it->second].pinCount++;
}

void BufferManager::unpinPage(int fileId, int pageId) {
    PageId pid{fileId, pageId};
    auto it = pageTable_.find(pid);
    if (it == pageTable_.end()) throw std::runtime_error("Page not in buffer pool");
    Frame &f = frames_[it->second];
    if (f.pinCount <= 0) throw std::runtime_error("Unpin on page with pinCount=0");
    f.pinCount--;
}

void BufferManager::markDirty(int fileId, int pageId) {
    PageId pid{fileId, pageId};
    auto it = pageTable_.find(pid);
    if (it == pageTable_.end()) throw std::runtime_error("Page not in buffer pool");
    frames_[it->second].isDirty = true;
}

void BufferManager::flushPage(int fileId, int pageId) {
    PageId pid{fileId, pageId};
    auto it = pageTable_.find(pid);
    if (it == pageTable_.end()) return; // nothing to flush
    Frame &f = frames_[it->second];
    if (f.isValid && f.isDirty) {
        fm_.writePage(fileId, pageId, f.data);
        f.isDirty = false;
    }
}

void BufferManager::flushAllPages() {
    for (auto &f : frames_) {
        if (f.isValid && f.isDirty) {
            fm_.writePage(f.pid.fileId, f.pid.pageId, f.data);
            f.isDirty = false;
        }
    }
}

std::size_t BufferManager::selectVictimFrame() {
    for (std::size_t i = 0; i < poolSize_; ++i) {
        clockHand_ = (clockHand_ + 1) % poolSize_;
        Frame &f = frames_[clockHand_];
        if (!f.isValid) {
            return clockHand_;
        }
        if (f.pinCount == 0) {
            return clockHand_;
        }
    }
    throw std::runtime_error("All buffer frames are pinned; no victim available");
}