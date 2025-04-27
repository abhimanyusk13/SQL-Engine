#pragma once
#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#include "Record.h"  // for FieldValue

/// Holds all registered scalar functions
class FunctionRegistry {
public:
    /// A UDF takes N FieldValues, returns one FieldValue
    using UDF = std::function<FieldValue(const std::vector<FieldValue>&)>;

    /// Register a new function under `name` (case‐insensitive).
    /// Overwrites any existing registration.
    static void registerFunction(const std::string &name, UDF func);

    /// Returns true if a function by that name is registered.
    static bool hasFunction(const std::string &name);

    /// Get the UDF by name; throws std::runtime_error if not found.
    static const UDF &getFunction(const std::string &name);

private:
    static std::unordered_map<std::string, UDF> &registry();
};
