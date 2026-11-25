#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace dsa::json {

struct JsonValue {
    enum class Type { Null, Bool, Int, Double, String, Array, Object };

    Type type = Type::Null;
    bool boolVal = false;
    long long intVal = 0;
    double doubleVal = 0.0;
    std::string strVal;
    std::vector<JsonValue> arrVal;
    std::map<std::string, JsonValue> objVal;

    bool isNull() const noexcept { return type == Type::Null; }
    bool isObject() const noexcept { return type == Type::Object; }
    bool isArray() const noexcept { return type == Type::Array; }

    bool has(const std::string& key) const {
        return type == Type::Object && objVal.find(key) != objVal.end();
    }

    const JsonValue* getPtr(const std::string& key) const {
        if (type != Type::Object) return nullptr;
        auto it = objVal.find(key);
        return (it != objVal.end()) ? &it->second : nullptr;
    }

    std::string getString(const std::string& key, const std::string& fallback = "") const;
    long long getInt(const std::string& key, long long fallback = 0, bool required = false) const;
    double getNumber(const std::string& key, double fallback = 0.0) const;
    bool getBool(const std::string& key, bool fallback = false) const;
    std::vector<int> getIntArray(const std::string& key, bool required = false) const;
    std::vector<std::string> getStringArray(const std::string& key) const;
};

JsonValue parse(const std::string& src);
std::string escape(const std::string& s);
std::string intArray(const std::vector<int>& arr);
std::string stringArray(const std::vector<std::string>& arr);

} // namespace dsa::json
