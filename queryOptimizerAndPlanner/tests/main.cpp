#include <iostream>
#include "Schema.h"
#include "QueryEngine.h"

int main() {
    // 1) Create engine (uses ./catalog/catalog.meta)
    QueryEngine engine("./catalog");

    // 2) Define schemas
    Schema custSchema({
        {"id",   DataType::INT,    0},
        {"name", DataType::STRING, 32}
    });
    Schema orderSchema({
        {"order_id", DataType::INT, 0},
        {"cust_id",  DataType::INT, 0},
        {"amount",   DataType::INT, 0}
    });

    // 3) Register tables
    engine.registerTable("customers", custSchema,
                         "customers.dat", "customers.idx", "id");
    engine.registerTable("orders", orderSchema,
                         "orders.dat",    "orders.idx",    "order_id");

    // 4) Insert sample data
    engine.executeQuery("INSERT INTO customers (id, name) VALUES (1, 'Alice');");
    engine.executeQuery("INSERT INTO customers (id, name) VALUES (2, 'Bob');");
    engine.executeQuery("INSERT INTO orders    (order_id, cust_id, amount) VALUES (100, 1, 500);");
    engine.executeQuery("INSERT INTO orders    (order_id, cust_id, amount) VALUES (101, 2, 300);");
    engine.executeQuery("INSERT INTO orders    (order_id, cust_id, amount) VALUES (102, 1, 200);");

    // 5) Run a join query
    //    SELECT c.name, o.order_id, o.amount
    //      FROM customers AS c, orders AS o
    //     WHERE c.id = o.cust_id;
    auto rows = engine.executeQuery(R"sql(
        SELECT customers.name, orders.order_id, orders.amount
          FROM customers, orders
         WHERE customers.id = orders.cust_id;
    )sql");

    // 6) Print results
    std::cout << "name\torder_id\tamount\n";
    for (auto &row : rows) {
        std::string name    = std::get<std::string>(row[0]);
        int32_t     oid     = std::get<int32_t>  (row[1]);
        int32_t     amount  = std::get<int32_t>  (row[2]);
        std::cout
            << name      << '\t'
            << oid       << '\t'
            << amount    << '\n';
    }

    return 0;
}
