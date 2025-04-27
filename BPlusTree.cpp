// File: BPlusTree.cpp
#include "BPlusTree.h"

template<typename Key, typename Value>
BPlusTree<Key, Value>::BPlusTree() : root_(nullptr) {}

template<typename Key, typename Value>
void BPlusTree<Key, Value>::insert(const Key &key, const Value &value) {
    // TODO: B+ tree insert logic
}

template<typename Key, typename Value>
bool BPlusTree<Key, Value>::remove(const Key &key) {
    // TODO: delete key
    return false;
}

template<typename Key, typename Value>
Value BPlusTree<Key, Value>::find(const Key &key) {
    // TODO: search leaf
    return Value();
}

template<typename Key, typename Value>
std::vector<Value> BPlusTree<Key, Value>::rangeScan(const Key &low, const Key &high) {
    // TODO: traverse leaves
    return {};
}

// Explicit instantiation (if needed)
// template class BPlusTree<int, Record>;
