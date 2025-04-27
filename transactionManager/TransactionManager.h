// File: TransactionManager.h
#pragma once

#include <cstdint>
#include <unordered_set>
#include <stdexcept>

// Basic transaction states
enum class TxState { ACTIVE, COMMITTED, ABORTED };

class TransactionManager {
public:
    TransactionManager();

    // Begin a new transaction. Returns a unique transaction ID.
    int64_t beginTransaction();

    // Commit the given transaction. Throws if invalid state.
    void commitTransaction(int64_t txId);

    // Abort (rollback) the given transaction. Throws if invalid state.
    void abortTransaction(int64_t txId);

    // Get current state of a transaction.
    TxState getState(int64_t txId) const;

private:
    int64_t nextTxId_;
    std::unordered_map<int64_t, TxState> txTable_;
};
