// File: BPlusTree.h
#pragma once
#include <vector>

// Simplified B+ tree node
template<typename Key, typename Value>
class BPlusTree {
public:
    BPlusTree();
    void insert(const Key &key, const Value &value);
    bool remove(const Key &key);
    Value find(const Key &key);
    std::vector<Value> rangeScan(const Key &low, const Key &high);
private:
    struct Node; // TODO: define internal/leaf node structs with keys/children
    Node *root_;
};
