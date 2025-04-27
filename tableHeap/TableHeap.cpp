// File: TableHeap.cpp
#include "TableHeap.h"

TableHeap::TableHeap(FileManager &fm, BufferManager &bm,
                     const std::string &tableFile,
                     const Schema &schema)
    : fm_(fm), bm_(bm), schema_(schema) {
    // Compute sizes
    recordSize_ = schema_.getRecordSize();
    slotSize_   = recordSize_ + 1;  // 1 byte for tombstone
    maxSlotsPerPage_ =
        (int)((FileManager::PAGE_SIZE - sizeof(int)) / slotSize_);

    // Open or create the table file
    fileId_ = fm_.openFile(tableFile);
    // If empty, initialize first page
    if (fm_.getPageCount(fileId_) == 0) {
        int pid = fm_.allocatePage(fileId_);
        char *page = bm_.fetchPage(fileId_, pid);
        setNumSlots(page, 0);
        bm_.markDirty(fileId_, pid);
        bm_.unpinPage(fileId_, pid);
    }
}

RecordID TableHeap::insertRecord(const std::vector<FieldValue> &values) {
    Record rec(schema_, values);
    auto buf = rec.serialize();
    int pageCount = fm_.getPageCount(fileId_);
    // Try existing pages
    for (int pid = 0; pid < pageCount; ++pid) {
        char *page = bm_.fetchPage(fileId_, pid);
        int numSlots = getNumSlots(page);
        // Reuse a tombstoned slot
        for (int i = 0; i < numSlots; ++i) {
            char *slot = getSlotPtr(page, i);
            if (!isSlotAlive(slot)) {
                setSlotAlive(slot, true);
                std::memcpy(slot + 1, buf.data(), recordSize_);
                bm_.markDirty(fileId_, pid);
                bm_.unpinPage(fileId_, pid);
                return {pid, i};
            }
        }
        // Append new slot if space
        if (numSlots < maxSlotsPerPage_) {
            setNumSlots(page, numSlots + 1);
            char *slot = getSlotPtr(page, numSlots);
            setSlotAlive(slot, true);
            std::memcpy(slot + 1, buf.data(), recordSize_);
            bm_.markDirty(fileId_, pid);
            bm_.unpinPage(fileId_, pid);
            return {pid, numSlots};
        }
        bm_.unpinPage(fileId_, pid);
    }
    // No space: allocate new page
    int pid = fm_.allocatePage(fileId_);
    char *page = bm_.fetchPage(fileId_, pid);
    setNumSlots(page, 1);
    char *slot = getSlotPtr(page, 0);
    setSlotAlive(slot, true);
    std::memcpy(slot + 1, buf.data(), recordSize_);
    bm_.markDirty(fileId_, pid);
    bm_.unpinPage(fileId_, pid);
    return {pid, 0};
}

bool TableHeap::deleteRecord(const RecordID &rid) {
    if (rid.pageId < 0 || rid.slotNum < 0) return false;
    if (rid.pageId >= fm_.getPageCount(fileId_)) return false;
    char *page = bm_.fetchPage(fileId_, rid.pageId);
    int numSlots = getNumSlots(page);
    if (rid.slotNum >= numSlots) {
        bm_.unpinPage(fileId_, rid.pageId);
        return false;
    }
    char *slot = getSlotPtr(page, rid.slotNum);
    if (!isSlotAlive(slot)) {
        bm_.unpinPage(fileId_, rid.pageId);
        return false;
    }
    setSlotAlive(slot, false);
    bm_.markDirty(fileId_, rid.pageId);
    bm_.unpinPage(fileId_, rid.pageId);
    return true;
}

bool TableHeap::updateRecord(const RecordID &rid,
                             const std::vector<FieldValue> &values) {
    if (rid.pageId < 0 || rid.slotNum < 0) return false;
    if (rid.pageId >= fm_.getPageCount(fileId_)) return false;
    Record rec(schema_, values);
    auto buf = rec.serialize();
    char *page = bm_.fetchPage(fileId_, rid.pageId);
    int numSlots = getNumSlots(page);
    if (rid.slotNum >= numSlots) {
        bm_.unpinPage(fileId_, rid.pageId);
        return false;
    }
    char *slot = getSlotPtr(page, rid.slotNum);
    if (!isSlotAlive(slot)) {
        bm_.unpinPage(fileId_, rid.pageId);
        return false;
    }
    std::memcpy(slot + 1, buf.data(), recordSize_);
    bm_.markDirty(fileId_, rid.pageId);
    bm_.unpinPage(fileId_, rid.pageId);
    return true;
}

std::vector<RecordID> TableHeap::tableScan() {
    std::vector<RecordID> results;
    int pageCount = fm_.getPageCount(fileId_);
    for (int pid = 0; pid < pageCount; ++pid) {
        char *page = bm_.fetchPage(fileId_, pid);
        int numSlots = getNumSlots(page);
        for (int i = 0; i < numSlots; ++i) {
            char *slot = getSlotPtr(page, i);
            if (isSlotAlive(slot)) results.push_back({pid, i});
        }
        bm_.unpinPage(fileId_, pid);
    }
    return results;
}

// --- Private helpers ---
int TableHeap::getNumSlots(char *pageData) const {
    int num;
    std::memcpy(&num, pageData, sizeof(num));
    return num;
}

void TableHeap::setNumSlots(char *pageData, int numSlots) {
    std::memcpy(pageData, &numSlots, sizeof(numSlots));
}

char *TableHeap::getSlotPtr(char *pageData, int slotIdx) const {
    return pageData + sizeof(int) + slotIdx * slotSize_;
}

bool TableHeap::isSlotAlive(char *slotPtr) const {
    return slotPtr[0] != 0;
}

void TableHeap::setSlotAlive(char *slotPtr, bool alive) const {
    slotPtr[0] = alive ? 1 : 0;
}

Record TableHeap::getRecord(const RecordID &rid) const {
    // Fetch page
    char *page = bm_.fetchPage(fileId_, rid.pageId);
    int numSlots = getNumSlots(page);
    if (rid.slotNum < 0 || rid.slotNum >= numSlots)
        throw std::runtime_error("Invalid RecordID: slot out of range");
    char *slot = getSlotPtr(page, rid.slotNum);
    if (!isSlotAlive(slot)) {
        bm_.unpinPage(fileId_, rid.pageId);
        throw std::runtime_error("Attempt to read deleted record");
    }
    // Deserialize
    Record rec = Record::deserialize(schema_, slot + 1);
    bm_.unpinPage(fileId_, rid.pageId);
    return rec;
}