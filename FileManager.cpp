// File: FileManager.cpp
#include "FileManager.h"
#include <stdexcept>

int FileManager::openFile(const std::string &filePath) {
    int fid = nextFileId_++;
    FileEntry entry;
    
    // Try opening existing file
    entry.stream.open(filePath, std::ios::in | std::ios::out | std::ios::binary);
    if (!entry.stream.is_open()) {
        // Create new file if not exists
        std::ofstream create(filePath, std::ios::out | std::ios::binary);
        create.close();
        entry.stream.open(filePath, std::ios::in | std::ios::out | std::ios::binary);
        if (!entry.stream.is_open()) {
            throw std::runtime_error("Failed to create file: " + filePath);
        }
    }

    files_.emplace(fid, std::move(entry));
    return fid;
}

void FileManager::closeFile(int fileId) {
    auto it = files_.find(fileId);
    if (it == files_.end()) throw std::runtime_error("Invalid fileId");
    it->second.stream.close();
    files_.erase(it);
}

void FileManager::readPage(int fileId, int pageId, char *buffer) {
    auto it = files_.find(fileId);
    if (it == files_.end()) throw std::runtime_error("Invalid fileId");
    FileEntry &e = it->second;

    int totalPages = getPageCount(fileId);
    if (pageId < 0 || pageId >= totalPages) {
        // Out-of-bounds: return zeroed page
        std::fill(buffer, buffer + PAGE_SIZE, 0);
        return;
    }

    std::streampos offset = static_cast<std::streampos>(pageId) * PAGE_SIZE;
    e.stream.seekg(offset, std::ios::beg);
    e.stream.read(buffer, PAGE_SIZE);
    if (e.stream.gcount() < static_cast<std::streamsize>(PAGE_SIZE)) {
        // Partial read: zero the rest
        std::fill(buffer + e.stream.gcount(), buffer + PAGE_SIZE, 0);
    }
}

void FileManager::writePage(int fileId, int pageId, const char *buffer) {
    auto it = files_.find(fileId);
    if (it == files_.end()) throw std::runtime_error("Invalid fileId");
    FileEntry &e = it->second;

    std::streampos offset = static_cast<std::streampos>(pageId) * PAGE_SIZE;
    e.stream.seekp(offset, std::ios::beg);
    e.stream.write(buffer, PAGE_SIZE);
    e.stream.flush();
}

int FileManager::getPageCount(int fileId) {
    auto it = files_.find(fileId);
    if (it == files_.end()) throw std::runtime_error("Invalid fileId");
    FileEntry &e = it->second;

    std::streampos current = e.stream.tellg();
    e.stream.seekg(0, std::ios::end);
    std::streampos fileSize = e.stream.tellg();
    e.stream.seekg(current);

    return static_cast<int>(fileSize / PAGE_SIZE);
}

int FileManager::allocatePage(int fileId) {
    auto it = files_.find(fileId);
    if (it == files_.end()) throw std::runtime_error("Invalid fileId");
    FileEntry &e = it->second;

    if (!e.freeList.empty()) {
        int pid = e.freeList.back();
        e.freeList.pop_back();
        return pid;
    }
    int newPage = getPageCount(fileId);
    // Initialize new page to zeros
    std::vector<char> zeros(PAGE_SIZE, 0);
    writePage(fileId, newPage, zeros.data());
    return newPage;
}

void FileManager::deallocatePage(int fileId, int pageId) {
    auto it = files_.find(fileId);
    if (it == files_.end()) throw std::runtime_error("Invalid fileId");
    if (pageId < 0) throw std::runtime_error("Invalid pageId");
    it->second.freeList.push_back(pageId);
}