// File: LockManager.h
#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <stdexcept>

/// Exception thrown on lock conflicts
class LockConflictException : public std::runtime_error {
public:
    LockConflictException(const std::string &msg)
      : std::runtime_error(msg) {}
};

/// Simple 2PL Lock Manager: shared (S) & exclusive (X) locks
class LockManager {
public:
    /// Acquire a shared lock on `resource` for transaction `txId`.
    /// Throws LockConflictException if an exclusive lock by another tx exists.
    void lockShared(int64_t txId, const std::string &resource);

    /// Acquire an exclusive lock on `resource` for transaction `txId`.
    /// Throws LockConflictException if any other tx holds S or X on the resource.
    void lockExclusive(int64_t txId, const std::string &resource);

    /// Release whichever lock (S or X) `txId` holds on `resource`.
    void unlock(int64_t txId, const std::string &resource);

    /// Release *all* locks held by `txId` (call on COMMIT or ROLLBACK).
    void releaseTransactionLocks(int64_t txId);

private:
    struct LockInfo {
        int64_t               xHolder     = 0;           // 0 = none
        std::unordered_set<int64_t> sHolders;             // txIds with shared locks
    };

    std::mutex                                       latch_;
    std::unordered_map<std::string, LockInfo>        table_;
};
