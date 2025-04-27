// File: QueryEngine.h
#pragma once

#include <string>
#include <vector>
#include "Parser.h"
#include "Binder.h"
#include "Planner.h"
#include "Rewriter.h"
#include "Optimizer.h"
#include "PhysicalPlanGenerator.h"
#include "Executor.h"
#include "StorageEngine.h"
#include "TransactionManager.h"
#include "LockManager.h"
#include "WALManager.h"
#include "Catalog.h"
#include "FileManager.h"
#include "BufferManager.h"
#include "StatsManager.h"

namespace physical { using Row = std::vector<FieldValue>; }

class QueryEngine {
public:
    explicit QueryEngine(const std::string &catalogDir);

    std::vector<physical::Row> executeQuery(const std::string &sql);

    void registerTable(const std::string &name,
                       const Schema &schema,
                       const std::string &dataFile,
                       const std::string &indexFile,
                       const std::string &pkColumn);

private:
    FileManager           fm_;
    BufferManager         bm_;
    Catalog               catalog_;
    LockManager           lockMgr_;
    WALManager            walMgr_;
    StorageEngine         storage_;
    TransactionManager    txMgr_;
    StatsManager          statsMgr_;

    Parser                parser_;
    Binder                binder_;
    Planner               planner_;
    Rewriter              rewriter_;
    Optimizer             optimizer_;
    PhysicalPlanGenerator physGen_;
    Executor              executor_;

    int64_t               currentTxId_ = 0;
};
