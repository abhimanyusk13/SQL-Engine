#include <iostream>
#include "QueryEngine.h"

int main() {
    QueryEngine engine("./catalog");

    // Register a simple users table
    engine.registerTable("users",
        Schema({{"id", DataType::INT, 0},
                {"name", DataType::STRING, 32}}),
        "users.dat", "users.idx", "id");

    auto run = [&](const std::string &sql){
        try {
            auto rows = engine.executeQuery(sql);
            if (!rows.empty()) {
                std::cout << "[RESULT] ";
                for (auto &fv : rows[0]) {
                    if (std::holds_alternative<int32_t>(fv))
                        std::cout << std::get<int32_t>(fv) << "\t";
                    else
                        std::cout << std::get<std::string>(fv) << "\t";
                }
                std::cout << "\n";
            } else {
                std::cout << "[OK] " << sql << "\n";
            }
        } catch (auto &e) {
            std::cout << "[ERROR] " << sql << " -> " << e.what() << "\n";
        }
    };

    // 1) DML outside TX should error
    run("INSERT INTO users (id,name) VALUES (1,'Alice')");

    // 2) Proper TX w/ commit
    run("BEGIN");
    run("INSERT INTO users (id,name) VALUES (1,'Alice')");
    run("INSERT INTO users (id,name) VALUES (2,'Bob')");
    run("COMMIT");

    // 3) read back
    run("SELECT id, name FROM users");

    // 4) TX with rollback
    run("BEGIN");
    run("INSERT INTO users (id,name) VALUES (3,'Charlie')");
    run("DELETE FROM users WHERE id = 1");
    run("ROLLBACK");

    // 5) confirm no change
    run("SELECT id, name FROM users");

    return 0;
}