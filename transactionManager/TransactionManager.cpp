// File: TransactionManager.cpp
#include "TransactionManager.h"

TransactionManager::TransactionManager()
    : nextTxId_(1) {}

int64_t TransactionManager::beginTransaction() {
    int64_t txId = nextTxId_++;
    txTable_[txId] = TxState::ACTIVE;
    return txId;
}

void TransactionManager::commitTransaction(int64_t txId) {
    auto it = txTable_.find(txId);
    if (it == txTable_.end()) throw std::runtime_error("Invalid transaction ID");
    if (it->second != TxState::ACTIVE)
        throw std::runtime_error("Transaction not active, cannot commit");
    // TODO: flush WAL up to commit, release locks
    it->second = TxState::COMMITTED;
}

void TransactionManager::abortTransaction(int64_t txId) {
    auto it = txTable_.find(txId);
    if (it == txTable_.end()) throw std::runtime_error("Invalid transaction ID");
    if (it->second != TxState::ACTIVE)
        throw std::runtime_error("Transaction not active, cannot abort");
    // TODO: undo changes via WAL, release locks
    it->second = TxState::ABORTED;
}

TxState TransactionManager::getState(int64_t txId) const {
    auto it = txTable_.find(txId);
    if (it == txTable_.end()) throw std::runtime_error("Invalid transaction ID");
    return it->second;
}