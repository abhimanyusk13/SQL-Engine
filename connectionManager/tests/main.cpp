// File: main.cpp
#include <cstdlib>
#include <iostream>
#include "FileManager.h"
#include "BufferManager.h"
#include "Catalog.h"
#include "LockManager.h"
#include "WALManager.h"
#include "StorageEngine.h"
#include "QueryEngine.h"
#include "ConnectionManager.h"

int main(int argc, char *argv[]) {
    int port = 5432;
    if (argc > 1) port = std::atoi(argv[1]);

    FileManager   fm;
    BufferManager bm(fm, /*poolSize=*/128);
    Catalog       catalog("./catalog");
    LockManager   lockMgr;
    WALManager    walMgr("./catalog/wal.log");
    StorageEngine storage(fm, bm, catalog, lockMgr, walMgr);
    QueryEngine   engine(catalog, fm, bm, lockMgr, walMgr, storage);
    ConnectionManager connMgr(engine, port);

    connMgr.run();
    return 0;
}

/*
$ telnet localhost 5432
INSERT INTO users (id,name) VALUES (1,'Alice');
OK
BEGIN;
INSERT INTO users (id,name) VALUES (2,'Bob');
COMMIT;
SELECT id, name FROM users;
1    Alice
2    Bob

engine will serve multiple statements over TCP
*/