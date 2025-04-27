// File: FileManager.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstddef>

class FileManager {
public:
    static constexpr std::size_t PAGE_SIZE = 8192;

    // Opens an existing file or creates a new one. Returns an integer file handle.
    int openFile(const std::string &filePath);
    // Closes the file associated with the given handle.
    void closeFile(int fileId);

    // Reads a full page (PAGE_SIZE bytes) at pageId into the provided buffer.
    // If pageId >= current page count, buffer is zeroed.
    void readPage(int fileId, int pageId, char *buffer);
    // Writes a full page (PAGE_SIZE bytes) from the provided buffer into pageId.
    void writePage(int fileId, int pageId, const char *buffer);

    // Allocates a new page, either by reusing a freed page or appending at end.
    // Returns the allocated pageId.
    int allocatePage(int fileId);
    // Marks a pageId as free for future reuse.
    void deallocatePage(int fileId, int pageId);

    // Returns the total number of pages currently in the file (including those freed).
    int getPageCount(int fileId);

private:
    struct FileEntry {
        std::fstream stream;
        std::vector<int> freeList;
    };

    std::unordered_map<int, FileEntry> files_;
    int nextFileId_ = 1;
};