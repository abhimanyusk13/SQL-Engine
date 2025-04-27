// File: BPlusTree.h
#pragma once

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include "BufferManager.h"

// A simple B+ tree storing Key->Value mappings in fixed-size pages
// Splits on insert; delete only removes from leaf (no rebalance).

template<typename Key, typename Value>
class BPlusTree {
public:
    // Initialize tree stored in fileId via buffer manager
    BPlusTree(int fileId, BufferManager &bm);
    ~BPlusTree();

    // Insert or update a key/value
    void insert(const Key &key, const Value &value);
    // Remove a key (only from leaf, may underflow)
    bool remove(const Key &key);
    // Find a key's value; returns true if found
    bool find(const Key &key, Value &out) const;
    // Range scan inclusive of low and high
    std::vector<Value> rangeScan(const Key &low, const Key &high) const;

private:
    struct Node {
        bool isLeaf;
        int numKeys;
        Key keys[MAX_KEYS];
        union {
            struct {
                Value values[MAX_KEYS];
                int next;
            } leaf;
            int children[MAX_KEYS+1];
        } ptr;
    };

    static constexpr int HEADER_PAGE = 0;
    static constexpr int MAX_KEYS = 128;
    static_assert((int)sizeof(Node) <= FileManager::PAGE_SIZE,
                  "Node size exceeds page size");

    int fileId_;
    BufferManager &bm_;
    int rootPage_;

    // Load/store header (root pointer)
    void loadHeader();
    void writeHeader();

    // Node serialization
    void readNode(int pageId, Node &node) const;
    void writeNode(int pageId, const Node &node);

    // Node allocation
    int allocatePage();

    // Recursive insert: returns {splitKey, newPage} if split, or invalid
    struct SplitResult { bool split; Key key; int pageId; };
    SplitResult insertRecursive(int pageId, const Key &key, const Value &value);

    // Helper to find leaf page for a given key
    int findLeafPage(int pageId, const Key &key) const;
};

// Implementation

template<typename Key, typename Value>
BPlusTree<Key,Value>::BPlusTree(int fileId, BufferManager &bm)
    : fileId_(fileId), bm_(bm) {
    // On first use, if no pages, create header + empty leaf root
    int pages = bm_.fetchPage(fileId_, 0), dummy = 0; bm_.unpinPage(fileId_,0);
    pages = (int)bm_.fetchPage(fileId_,0); // just to call getPageCount
    bm_.unpinPage(fileId_,0);
    int count = bm_.fetchPage(fileId_,0)?0:0; (void)count;
    int pageCount = bm_.bm_.fm_.getPageCount(fileId_);
    if (pageCount == 0) {
        // Create header page
        allocatePage();
        // Create first leaf root
        Node root{};
        root.isLeaf = true;
        root.numKeys = 0;
        root.ptr.leaf.next = -1;
        int p = allocatePage();
        writeNode(p, root);
        rootPage_ = p;
        writeHeader();
    } else {
        loadHeader();
    }
}

template<typename Key, typename Value>
BPlusTree<Key,Value>::~BPlusTree() {
    // nothing
}

template<typename Key, typename Value>
void BPlusTree<Key,Value>::loadHeader() {
    char *buf = bm_.fetchPage(fileId_, HEADER_PAGE);
    std::memcpy(&rootPage_, buf, sizeof(rootPage_));
    bm_.unpinPage(fileId_, HEADER_PAGE);
}

template<typename Key, typename Value>
void BPlusTree<Key,Value>::writeHeader() {
    char *buf = bm_.fetchPage(fileId_, HEADER_PAGE);
    std::memcpy(buf, &rootPage_, sizeof(rootPage_));
    bm_.markDirty(fileId_, HEADER_PAGE);
    bm_.unpinPage(fileId_, HEADER_PAGE);
}

template<typename Key, typename Value>
void BPlusTree<Key,Value>::readNode(int pageId, Node &node) const {
    char *buf = bm_.fetchPage(fileId_, pageId);
    std::memcpy(&node, buf, sizeof(Node));
    bm_.unpinPage(fileId_, pageId);
}

template<typename Key, typename Value>
void BPlusTree<Key,Value>::writeNode(int pageId, const Node &node) {
    char *buf = bm_.fetchPage(fileId_, pageId);
    std::memcpy(buf, &node, sizeof(Node));
    bm_.markDirty(fileId_, pageId);
    bm_.unpinPage(fileId_, pageId);
}

template<typename Key, typename Value>
int BPlusTree<Key,Value>::allocatePage() {
    int pid = bm_.bm_.fm_.allocatePage(fileId_);
    return pid;
}

template<typename Key, typename Value>
bool BPlusTree<Key,Value>::find(const Key &key, Value &out) const {
    if (rootPage_ < 0) return false;
    int leaf = findLeafPage(rootPage_, key);
    Node node;
    readNode(leaf, node);
    for (int i = 0; i < node.numKeys; ++i) {
        if (node.keys[i] == key) {
            out = node.ptr.leaf.values[i];
            return true;
        }
    }
    return false;
}

template<typename Key, typename Value>
int BPlusTree<Key,Value>::findLeafPage(int pageId, const Key &key) const {
    Node node;
    readNode(pageId, node);
    if (node.isLeaf) return pageId;
    int i = 0;
    while (i < node.numKeys && key >= node.keys[i]) ++i;
    int child = node.ptr.children[i];
    return findLeafPage(child, key);
}

// SplitResult handling
template<typename Key, typename Value>
auto BPlusTree<Key,Value>::insertRecursive(int pageId, const Key &key, const Value &value)
    -> SplitResult {
    Node node;
    readNode(pageId, node);
    if (node.isLeaf) {
        // Insert into leaf
        int pos = std::lower_bound(node.keys, node.keys + node.numKeys, key) - node.keys;
        if (pos < node.numKeys && node.keys[pos] == key) {
            node.ptr.leaf.values[pos] = value;
            writeNode(pageId, node);
            return {false, Key(), -1};
        }
        for (int i = node.numKeys; i > pos; --i) {
            node.keys[i] = node.keys[i-1];
            node.ptr.leaf.values[i] = node.ptr.leaf.values[i-1];
        }
        node.keys[pos] = key;
        node.ptr.leaf.values[pos] = value;
        node.numKeys++;
        if (node.numKeys <= MAX_KEYS) {
            writeNode(pageId, node);
            return {false, Key(), -1};
        }
        // Split leaf
        int split = node.numKeys / 2;
        Node newLeaf{};
        newLeaf.isLeaf = true;
        newLeaf.numKeys = node.numKeys - split;
        for (int i = 0; i < newLeaf.numKeys; ++i) {
            newLeaf.keys[i] = node.keys[split + i];
            newLeaf.ptr.leaf.values[i] = node.ptr.leaf.values[split + i];
        }
        newLeaf.ptr.leaf.next = node.ptr.leaf.next;
        node.numKeys = split;
        node.ptr.leaf.next = allocatePage();
        writeNode(pageId, node);
        writeNode(node.ptr.leaf.next, newLeaf);
        return {true, newLeaf.keys[0], node.ptr.leaf.next};
    } else {
        // Internal node
        int idx = std::upper_bound(node.keys, node.keys + node.numKeys, key) - node.keys;
        int childId = node.ptr.children[idx];
        SplitResult res = insertRecursive(childId, key, value);
        if (!res.split) return {false, Key(), -1};
        // Insert promoted key
        int pos = std::lower_bound(node.keys, node.keys + node.numKeys, res.key) - node.keys;
        for (int i = node.numKeys; i > pos; --i) {
            node.keys[i] = node.keys[i-1];
            node.ptr.children[i+1] = node.ptr.children[i];
        }
        node.keys[pos] = res.key;
        node.ptr.children[pos+1] = res.pageId;
        node.numKeys++;
        if (node.numKeys <= MAX_KEYS) {
            writeNode(pageId, node);
            return {false, Key(), -1};
        }
        // Split internal
        int split = node.numKeys / 2;
        Key upKey = node.keys[split];
        Node newInt{};
        newInt.isLeaf = false;
        newInt.numKeys = node.numKeys - split - 1;
        for (int i = 0; i < newInt.numKeys; ++i) {
            newInt.keys[i] = node.keys[split + 1 + i];
            newInt.ptr.children[i] = node.ptr.children[split + 1 + i];
        }
        newInt.ptr.children[newInt.numKeys] = node.ptr.children[node.numKeys + 1];
        node.numKeys = split;
        writeNode(pageId, node);
        int newPage = allocatePage();
        writeNode(newPage, newInt);
        return {true, upKey, newPage};
    }
}

// Public insert
template<typename Key, typename Value>
void BPlusTree<Key,Value>::insert(const Key &key, const Value &value) {
    SplitResult res = insertRecursive(rootPage_, key, value);
    if (res.split) {
        // Create new root
        Node newRoot{};
        newRoot.isLeaf = false;
        newRoot.numKeys = 1;
        newRoot.keys[0] = res.key;
        newRoot.ptr.children[0] = rootPage_;
        newRoot.ptr.children[1] = res.pageId;
        int newRootPage = allocatePage();
        writeNode(newRootPage, newRoot);
        rootPage_ = newRootPage;
        writeHeader();
    }
}

// Delete from leaf only
template<typename Key, typename Value>
bool BPlusTree<Key,Value>::remove(const Key &key) {
    int leaf = findLeafPage(rootPage_, key);
    Node node;
    readNode(leaf, node);
    int pos = std::lower_bound(node.keys, node.keys + node.numKeys, key) - node.keys;
    if (pos >= node.numKeys || node.keys[pos] != key) return false;
    for (int i = pos; i < node.numKeys - 1; ++i) {
        node.keys[i] = node.keys[i+1];
        node.ptr.leaf.values[i] = node.ptr.leaf.values[i+1];
    }
    node.numKeys--;
    writeNode(leaf, node);
    return true;
}

// Range scan
template<typename Key, typename Value>
std::vector<Value> BPlusTree<Key,Value>::rangeScan(const Key &low, const Key &high) const {
    std::vector<Value> result;
    if (rootPage_ < 0) return result;
    int leaf = findLeafPage(rootPage_, low);
    Node node;
    readNode(leaf, node);
    while (true) {
        for (int i = 0; i < node.numKeys; ++i) {
            if (node.keys[i] >= low && node.keys[i] <= high) {
                result.push_back(node.ptr.leaf.values[i]);
            } else if (node.keys[i] > high) {
                return result;
            }
        }
        if (node.ptr.leaf.next < 0) break;
        leaf = node.ptr.leaf.next;
        readNode(leaf, node);
    }
    return result;
}


// File: BPlusTree.cpp
#include "BPlusTree.h"
// Template implementation is in header. No CPP code needed. You can add explicit instantiations here if desired.
