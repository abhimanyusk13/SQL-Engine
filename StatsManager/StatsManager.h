// File: StatsManager.h
#pragma once

#include <string>
#include "StorageEngine.h"
#include "Catalog.h"

/// Collects table‐level statistics and pushes them into the Catalog.
class StatsManager {
public:
    StatsManager(StorageEngine &storage, Catalog &catalog);

    /// Runs a full scan of `tableName`, computes row count and
    /// distinct‐value counts per column, and updates the catalog.
    void analyzeTable(const std::string &tableName);

private:
    StorageEngine &storage_;
    Catalog       &catalog_;
};


/*
ANALYZE users;
SELECT * FROM users;  -- now optimizer has up-to-date stats
*/