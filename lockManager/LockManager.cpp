// File: LockManager.cpp
#include "LockManager.h"

void LockManager::lockShared(int64_t txId, const std::string &resource) {
    std::lock_guard<std::mutex> guard(latch_);
    auto &info = table_[resource];

    // Conflict if another tx holds X
    if (info.xHolder != 0 && info.xHolder != txId) {
        throw LockConflictException(
            "S-lock conflict on [" + resource +
            "]: held X by tx " + std::to_string(info.xHolder));
    }
    // Grant S-lock
    info.sHolders.insert(txId);
}

void LockManager::lockExclusive(int64_t txId, const std::string &resource) {
    std::lock_guard<std::mutex> guard(latch_);
    auto &info = table_[resource];

    // Conflict if another tx holds X
    if (info.xHolder != 0 && info.xHolder != txId) {
        throw LockConflictException(
            "X-lock conflict on [" + resource +
            "]: held X by tx " + std::to_string(info.xHolder));
    }
    // Conflict if any other tx holds S
    for (auto other : info.sHolders) {
        if (other != txId) {
            throw LockConflictException(
                "X-lock conflict on [" + resource +
                "]: held S by tx " + std::to_string(other));
        }
    }
    // Promote/downgrade: remove S-lock by this tx if present
    info.sHolders.erase(txId);
    // Grant X-lock
    info.xHolder = txId;
}

void LockManager::unlock(int64_t txId, const std::string &resource) {
    std::lock_guard<std::mutex> guard(latch_);
    auto it = table_.find(resource);
    if (it == table_.end()) return;
    auto &info = it->second;

    if (info.xHolder == txId) {
        info.xHolder = 0;
    }
    info.sHolders.erase(txId);

    if (info.xHolder == 0 && info.sHolders.empty()) {
        table_.erase(it);
    }
}

void LockManager::releaseTransactionLocks(int64_t txId) {
    std::lock_guard<std::mutex> guard(latch_);
    for (auto it = table_.begin(); it != table_.end(); ) {
        auto &info = it->second;
        if (info.xHolder == txId) {
            info.xHolder = 0;
        }
        info.sHolders.erase(txId);

        if (info.xHolder == 0 && info.sHolders.empty()) {
            it = table_.erase(it);
        } else {
            ++it;
        }
    }
}
