#include "FileManager.h"
#include "BufferManager.h"
#include "Catalog.h"
#include "StorageEngine.h"
#include "PhysicalOperator.h"
#include "TableScan.h"
#include "Filter.h"
#include "Project.h"
#include "Executor.h"
#include "AST.h"

int main() {
    // 1) Bootstrap storage/catalog
    FileManager fm;
    BufferManager bm(fm, /*poolSize=*/128);
    Catalog catalog;
    catalog.load("./catalog");
    StorageEngine se(fm, bm);

    // 2) (Re)register a table called "users" with schema (id INT, name STRING(32)), PK=id
    Schema userSchema({
        {"id",   DataType::INT,    0},
        {"name", DataType::STRING, 32}
    });
    se.registerTable("users",
                     userSchema,
                     "users.dat",   // data file
                     "users.idx",   // index file
                     "id");         // primary‐key column

    // 3) Build a physical plan for:
    //      SELECT id, name FROM users WHERE id > 10;

    // 3a) TableScan on "users"
    auto *scan = new TableScan(se, catalog, "users");
    scan->open();      // prepares scan.colIdx_ internally
    scan->close();     // we only needed to populate its col‐map

    // grab the column‐to‐slot mapping that TableScan built
    auto colIdx = scan->colIdx_;  // (in a real engine you'd expose a getter)

    // 3b) Build predicate Expr: users.id > 10
    auto pred = std::make_unique<Expr>();
    pred->type     = Expr::Type::BINARY_OP;
    pred->op       = ">";
    pred->left     = std::make_unique<Expr>();
    pred->left->type       = Expr::Type::COLUMN_REF;
    pred->left->columnName = "users.id";
    pred->right    = std::make_unique<Expr>();
    pred->right->type      = Expr::Type::INT_LITERAL;
    pred->right->intValue  = 10;

    // 3c) Filter
    auto *filter = new Filter(scan, pred.get(), colIdx);

    // 3d) Project id and name
    std::vector<Expr*> projExprs;
    // id
    auto *e1 = new Expr(); e1->type = Expr::Type::COLUMN_REF; e1->columnName = "users.id";
    // name
    auto *e2 = new Expr(); e2->type = Expr::Type::COLUMN_REF; e2->columnName = "users.name";
    projExprs.push_back(e1);
    projExprs.push_back(e2);

    auto *proj = new Project(filter, projExprs, colIdx);

    // 4) Execute
    auto rows = executor::Executor::execute(proj);

    // 5) Print results
    for (auto &row : rows) {
        // row[0] is id, row[1] is name
        int32_t id = std::get<int32_t>(row[0]);
        std::string name = std::get<std::string>(row[1]);
        std::cout << "id=" << id << "  name=" << name << "\n";
    }

    // 6) Cleanup
    delete proj;
    delete filter;
    delete scan;
    delete e1;
    delete e2;

    return 0;
}
