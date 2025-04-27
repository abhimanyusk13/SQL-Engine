// File: main.cpp
#include "ConnectionManager.h"
#include "Parser.h"
#include "Binder.h"
#include "Catalog.h"
#include "StorageEngine.h"
#include "Executor.h"

int main() {
    ConnectionManager conn;
    conn.init(5432);

    Catalog catalog;
    catalog.load("./catalog/");

    StorageEngine store;
    store.openTable("users");

    Parser parser;
    Binder binder;
    Executor exec(store);

    // Example loop (pseudo)
    // while(sql = conn.nextQuery()) {
    //   auto ast = parser.parse(sql);
    //   binder.bind(ast);
    //   auto rows = exec.execute(ast);
    //   conn.sendResult(rows);
    // }
    return 0;
}

// Dev commit