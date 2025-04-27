#include "FunctionRegistry.h"
#include <algorithm>

using namespace std;

static string normalize(const string &s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), ::toupper);
    return out;
}

unordered_map<string, FunctionRegistry::UDF> &
FunctionRegistry::registry() {
    static unordered_map<string, UDF> inst;
    return inst;
}

void FunctionRegistry::registerFunction(const string &name, UDF func) {
    registry()[normalize(name)] = std::move(func);
}

bool FunctionRegistry::hasFunction(const string &name) {
    return registry().count(normalize(name)) > 0;
}

const FunctionRegistry::UDF &
FunctionRegistry::getFunction(const string &name) {
    auto &reg = registry();
    auto it = reg.find(normalize(name));
    if (it == reg.end()) {
        throw runtime_error("Unknown function: " + name);
    }
    return it->second;
}
