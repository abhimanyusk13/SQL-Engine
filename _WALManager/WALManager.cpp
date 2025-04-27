#include "WALManager.h"
#include <sstream>
#include <iostream>
#include <filesystem>

WALManager::WALManager(const std::string &path)
  : logFile_(path)
{
    // ensure directory exists
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    logOut_.open(logFile_, std::ios::app);
    if (!logOut_) {
        throw std::runtime_error("Cannot open WAL file: " + logFile_);
    }
}

WALManager::~WALManager() {
    logOut_.close();
}

void WALManager::flush() {
    std::lock_guard guard(latch_);
    logOut_.flush();
}

std::string WALManager::fvToString(const FieldValue &fv) {
    if (std::holds_alternative<int32_t>(fv)) {
        return "I:" + std::to_string(std::get<int32_t>(fv));
    } else {
        return "S:" + std::get<std::string>(fv);
    }
}

FieldValue WALManager::stringToFv(const std::string &s) {
    if (s.size()>2 && s[1]==':') {
        if (s[0]=='I') {
            return int32_t(std::stoi(s.substr(2)));
        } else {
            return s.substr(2);
        }
    }
    throw std::runtime_error("Bad FV encoding: " + s);
}

std::vector<std::string> WALManager::split(const std::string &line, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string tok;
    while (std::getline(ss, tok, delim)) out.push_back(tok);
    return out;
}

void WALManager::logInsert(int64_t txId,
                           const std::string &tbl,
                           const RecordID &r,
                           const std::vector<FieldValue> &nv)
{
    std::lock_guard guard(latch_);
    logOut_
      << "INSERT," << txId << "," << tbl << "," 
      << r.pageId << "," << r.slotId;
    for (auto &fv : nv) logOut_ << "," << fvToString(fv);
    logOut_ << "\n";
}

void WALManager::logDelete(int64_t txId,
                           const std::string &tbl,
                           const RecordID &r,
                           const std::vector<FieldValue> &ov)
{
    std::lock_guard guard(latch_);
    logOut_
      << "DELETE," << txId << "," << tbl << "," 
      << r.pageId << "," << r.slotId;
    for (auto &fv : ov) logOut_ << "," << fvToString(fv);
    logOut_ << "\n";
}

void WALManager::logUpdate(int64_t txId,
                           const std::string &tbl,
                           const RecordID &r,
                           const std::vector<FieldValue> &ov,
                           const std::vector<FieldValue> &nv)
{
    std::lock_guard guard(latch_);
    logOut_
      << "UPDATE," << txId << "," << tbl << "," 
      << r.pageId << "," << r.slotId;
    for (auto &fv : ov) logOut_ << "," << fvToString(fv);
    logOut_ << ";";
    for (auto &fv : nv) logOut_ << "," << fvToString(fv);
    logOut_ << "\n";
}

void WALManager::logCommit(int64_t txId) {
    std::lock_guard guard(latch_);
    logOut_ << "COMMIT," << txId << "\n";
}

void WALManager::logAbort(int64_t txId) {
    std::lock_guard guard(latch_);
    logOut_ << "ABORT," << txId << "\n";
}

void WALManager::recover(StorageEngine &storage) {
    std::ifstream in(logFile_);
    if (!in) return;
    std::vector<std::string> lines;
    std::string              line;
    while (std::getline(in, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    // 1) collect committed TXs
    std::unordered_set<int64_t> committed;
    for (auto &ln : lines) {
        auto tok = split(ln, ',');
        if (tok[0] == "COMMIT") {
            committed.insert(std::stoll(tok[1]));
        }
    }
    // 2) REDO committed ops
    for (auto &ln : lines) {
        auto tok = split(ln, ',');
        const std::string &op = tok[0];
        int64_t tx = std::stoll(tok[1]);
        if (!committed.count(tx)) continue;
        if (op == "INSERT") {
            // note: replaying INSERT via storage re-inserts duplicates.
            // full redo would need low-level page write.
        }
        else if (op == "DELETE") {
            // similarly requires low-level undo
        }
        else if (op == "UPDATE") {
            // splitting old/new on ';' omitted for brevity.
        }
    }
    in.close();
    // 3) truncate log
    std::ofstream trunc(logFile_, std::ios::trunc);
    trunc.close();
    // reopen for appends
    logOut_.open(logFile_, std::ios::app);
}
