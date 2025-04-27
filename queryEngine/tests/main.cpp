#include <iostream>
#include "QueryEngine.h"

int main() {
    QueryEngine engine("./catalog");

    // Register a simple table: users(id INT PK, name STRING(32))
    engine.registerTable("users",
        Schema({{"id", DataType::INT, 0},
                {"name", DataType::STRING, 32}}),
        "users.dat", "users.idx", "id");

    auto run = [&](const std::string &sql){
        try {
            auto rows = engine.executeQuery(sql);
            if (!rows.empty()) {
                std::cout << "[RESULT] ";
                for (auto &r : rows) {
                    // print each field (int or string)
                    if (std::holds_alternative<int32_t>(r[0]))
                        std::cout << std::get<int32_t>(r[0]);
                    else
                        std::cout << std::get<std::string>(r[0]);
                    std::cout << "\t";
                }
                std::cout << "\n";
            } else {
                std::cout << "[OK] " << sql << "\n";
            }
        } catch (const std::exception &e) {
            std::cout << "[ERROR] " << sql << " → " << e.what() << "\n";
        }
    };

    // 1) Attempt DML outside transaction
    run("INSERT INTO users (id,name) VALUES (1,'Alice')");

    // 2) Proper transaction with commit
    run("BEGIN");
    run("INSERT INTO users (id,name) VALUES (1,'Alice')");
    run("INSERT INTO users (id,name) VALUES (2,'Bob')");
    run("COMMIT");

    // 3) SELECT to verify both rows are visible
    run("SELECT id, name FROM users");

    // 4) Transaction with rollback
    run("BEGIN");
    run("INSERT INTO users (id,name) VALUES (3,'Charlie')");
    run("DELETE FROM users WHERE id = 1");
    run("ROLLBACK");

    // 5) SELECT to show rollback had no effect
    run("SELECT id, name FROM users");

    // 6) Another wrong DML (outside TX)
    run("DELETE FROM users WHERE id = 2");

    return 0;
}
