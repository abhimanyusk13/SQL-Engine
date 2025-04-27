// File: WALManager.cpp
#include "WALManager.h"
#include <sstream>
#include <iostream>
#include <filesystem>

WALManager::WALManager(const std::string &path)
  : logFile_(path)
{
    std::filesystem::create_directories(
      std::filesystem::path(path).parent_path());
    logOut_.open(logFile_, std::ios::app);
    if (!logOut_)
        throw std::runtime_error("Cannot open WAL: " + logFile_);
}

WALManager::~WALManager() {
    logOut_.close();
}

void WALManager::flush() {
    std::lock_guard guard(latch_);
    logOut_.flush();
}

static std::string fvToString(const FieldValue &fv) {
    if (std::holds_alternative<int32_t>(fv))
        return "I:" + std::to_string(std::get<int32_t>(fv));
    else
        return "S:" + std::get<std::string>(fv);
}

void WALManager::logInsert(int64_t tx, const std::string &tbl,
                           const RecordID &rid,
                           const std::vector<FieldValue> &nv)
{
    std::lock_guard guard(latch_);
    logOut_ << "INSERT," << tx << "," << tbl
            << "," << rid.pageId << "," << rid.slotId;
    for (auto &v : nv)
        logOut_ << "," << fvToString(v);
    logOut_ << "\n";
}

void WALManager::logDelete(int64_t tx, const std::string &tbl,
                           const RecordID &rid,
                           const std::vector<FieldValue> &ov)
{
    std::lock_guard guard(latch_);
    logOut_ << "DELETE," << tx << "," << tbl
            << "," << rid.pageId << "," << rid.slotId;
    for (auto &v : ov)
        logOut_ << "," << fvToString(v);
    logOut_ << "\n";
}

void WALManager::logUpdate(int64_t tx, const std::string &tbl,
                           const RecordID &rid,
                           const std::vector<FieldValue> &ov,
                           const std::vector<FieldValue> &nv)
{
    std::lock_guard guard(latch_);
    logOut_ << "UPDATE," << tx << "," << tbl
            << "," << rid.pageId << "," << rid.slotId;
    // old
    for (auto &v : ov) logOut_ << "," << fvToString(v);
    // delimiter
    logOut_ << ";";
    // new
    for (auto &v : nv) logOut_ << "," << fvToString(v);
    logOut_ << "\n";
}

void WALManager::logCommit(int64_t tx) {
    std::lock_guard guard(latch_);
    logOut_ << "COMMIT," << tx << "\n";
}

void WALManager::logAbort(int64_t tx) {
    std::lock_guard guard(latch_);
    logOut_ << "ABORT," << tx << "\n";
}

std::vector<std::string> WALManager::split(const std::string &s, char d) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, d)) out.push_back(tok);
    return out;
}

FieldValue WALManager::parseFV(const std::string &tok) {
    // Format "I:123" or "S:hello"
    if (tok.size()>2 && tok[1]==':') {
        if (tok[0]=='I')
            return int32_t(std::stoi(tok.substr(2)));
        else
            return tok.substr(2);
    }
    throw std::runtime_error("Bad FV token: " + tok);
}

void WALManager::recover(StorageEngine &storage) {
    // 1) Read all log lines
    std::ifstream in(logFile_);
    if (!in) return;  // no log yet
    std::vector<LogRecord> records;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        // split op,tx,table,page,slot,[rest]
        auto parts = split(line, ',');
        auto op = parts[0];
        int64_t tx = std::stoll(parts[1]);

        if (op=="COMMIT" || op=="ABORT") {
            records.push_back({ op=="COMMIT" ? LogOp::COMMIT : LogOp::ABORT,
                                tx, "", {0,0}, {}, {} });
            continue;
        }

        std::string tbl = parts[2];
        RecordID rid{ std::stoll(parts[3]),
                      std::stoll(parts[4]) };

        if (op=="INSERT") {
            std::vector<FieldValue> nv;
            for (size_t i=5;i<parts.size();i++)
                nv.push_back(parseFV(parts[i]));
            records.push_back({ LogOp::INSERT, tx, tbl, rid, {}, nv });
        }
        else if (op=="DELETE") {
            std::vector<FieldValue> ov;
            for (size_t i=5;i<parts.size();i++)
                ov.push_back(parseFV(parts[i]));
            records.push_back({ LogOp::DELETE, tx, tbl, rid, ov, {} });
        }
        else if (op=="UPDATE") {
            // parts[5..] is old until a part starts with ';'
            std::vector<FieldValue> ov, nv;
            // split old/new on ';'
            auto semiPos = line.find(';');
            auto oldPart = line.substr(0, semiPos);
            auto newPart = line.substr(semiPos+1);
            auto oldToks = split(oldPart, ',');
            for (size_t i=5;i<oldToks.size();i++)
                ov.push_back(parseFV(oldToks[i]));
            auto newToks = split(newPart, ',');
            for (size_t i=1;i<newToks.size();i++)
                nv.push_back(parseFV(newToks[i]));
            records.push_back({ LogOp::UPDATE, tx, tbl, rid, ov, nv });
        }
    }
    in.close();

    // 2) Find all committed TXs
    std::unordered_set<int64_t> committed, aborted;
    for (auto &rec : records) {
        if (rec.op==LogOp::COMMIT)   committed.insert(rec.txId);
        if (rec.op==LogOp::ABORT)    aborted.insert(rec.txId);
    }

    // 3) REDO committed operations (in log order)
    for (auto &rec : records) {
        if (!committed.count(rec.txId)) continue;
        switch (rec.op) {
          case LogOp::INSERT:
            storage.redoInsert(rec.table, rec.rid, rec.newValues);
            break;
          case LogOp::DELETE:
            storage.redoDelete(rec.table, rec.rid);
            break;
          case LogOp::UPDATE:
            storage.redoUpdate(rec.table, rec.rid, rec.newValues);
            break;
          default: break;
        }
    }

    // 4) UNDO uncommitted (in reverse order)
    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        auto &rec = *it;
        if (committed.count(rec.txId) || aborted.count(rec.txId)) continue;
        // neither committed nor aborted: partial tx
        switch (rec.op) {
          case LogOp::INSERT:
            // undo insert = delete
            storage.redoDelete(rec.table, rec.rid);
            break;
          case LogOp::DELETE:
            // undo delete = re-insert old values
            storage.redoInsert(rec.table, rec.rid, rec.oldValues);
            break;
          case LogOp::UPDATE:
            // undo update = write oldValues
            storage.redoUpdate(rec.table, rec.rid, rec.oldValues);
            break;
          default: break;
        }
    }

    // 5) Truncate the log
    std::ofstream trunc(logFile_, std::ios::trunc);
    trunc.close();

    // reopen for appending
    logOut_.open(logFile_, std::ios::app);
}
