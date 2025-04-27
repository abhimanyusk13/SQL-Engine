// File: TableHeap.h
#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include "Schema.h"
#include "Record.h"
#include "BufferManager.h"
#include "FileManager.h"

// Identifier for a record within a table
struct RecordID {
    int pageId;
    int slotNum;
};

class TableHeap {
public:
    // Open or create a table file and initialize schema
    TableHeap(FileManager &fm, BufferManager &bm,
              const std::string &tableFile, const Schema &schema);
    ~TableHeap() = default;

    // Insert a record; returns its RecordID
    RecordID insertRecord(const std::vector<FieldValue> &values);
    // Delete a record by marking it tombstoned
    bool deleteRecord(const RecordID &rid);
    // Update an existing record in-place
    bool updateRecord(const RecordID &rid,
                      const std::vector<FieldValue> &values);
    // Scan all alive records and return their RecordIDs
    std::vector<RecordID> tableScan();
    // Fetch a record by RecordID
    Record getRecord(const RecordID &rid) const;

private:
    FileManager &fm_;
    BufferManager &bm_;
    int fileId_;
    Schema schema_;
    std::size_t recordSize_;     // bytes for record payload
    std::size_t slotSize_;       // 1 byte tombstone + recordSize_
    int maxSlotsPerPage_;        // computed from PAGE_SIZE

    // Page layout helpers
    int getNumSlots(char *pageData) const;
    void setNumSlots(char *pageData, int numSlots);
    char *getSlotPtr(char *pageData, int slotIdx) const;
    bool isSlotAlive(char *slotPtr) const;
    void setSlotAlive(char *slotPtr, bool alive) const;
};