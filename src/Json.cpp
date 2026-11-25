#include "dsa/Json.hpp"
#include "dsa/Types.hpp"

#include <cctype>
#include <limits>
#include <sstream>

namespace dsa::json {

std::string JsonValue::getString(const std::string& key, const std::string& fallback) const {
    if (!has(key)) return fallback;
    const auto& v = objVal.at(key);
    if (v.type == Type::Null) return fallback;
    if (v.type != Type::String) {
        throw ValidationException("Field '" + key + "' must be a string.");
    }
    return v.strVal;
}

long long JsonValue::getInt(const std::string& key, long long fallback, bool required) const {
    if (!has(key)) {
        if (required) throw ValidationException("Missing required integer field '" + key + "'.");
        return fallback;
    }
    const auto& v = objVal.at(key);
    if (v.type == Type::Int) return v.intVal;
    if (v.type == Type::Double) return static_cast<long long>(v.doubleVal);
    throw ValidationException("Field '" + key + "' must be an integer.");
}

double JsonValue::getNumber(const std::string& key, double fallback) const {
    if (!has(key)) return fallback;
    const auto& v = objVal.at(key);
    if (v.type == Type::Int) return static_cast<double>(v.intVal);
    if (v.type == Type::Double) return v.doubleVal;
    return fallback;
}

bool JsonValue::getBool(const std::string& key, bool fallback) const {
    if (!has(key)) return fallback;
    const auto& v = objVal.at(key);
    if (v.type == Type::Null) return fallback;
    if (v.type != Type::Bool) {
        throw ValidationException("Field '" + key + "' must be a boolean.");
    }
    return v.boolVal;
}

std::vector<int> JsonValue::getIntArray(const std::string& key, bool required) const {
    if (!has(key)) {
        if (required) throw ValidationException("Missing required array field '" + key + "'.");
        return {};
    }
    const auto& v = objVal.at(key);
    if (v.type != Type::Array) {
        throw ValidationException("Field '" + key + "' must be an array.");
    }
    std::vector<int> out;
    out.reserve(v.arrVal.size());
    for (const auto& elem : v.arrVal) {
        if (elem.type != Type::Int) {
            throw ValidationException("Array '" + key + "' must contain only integers.");
        }
        if (elem.intVal < std::numeric_limits<int>::min() ||
            elem.intVal > std::numeric_limits<int>::max()) {
            throw ValidationException("Integer value out of range in '" + key + "'.");
        }
        out.push_back(static_cast<int>(elem.intVal));
    }
    return out;
}

std::vector<std::string> JsonValue::getStringArray(const std::string& key) const {
    if (!has(key)) return {};
    const auto& v = objVal.at(key);
    if (v.type != Type::Array) {
        throw ValidationException("Field '" + key + "' must be an array.");
    }
    std::vector<std::string> out;
    out.reserve(v.arrVal.size());
    for (const auto& elem : v.arrVal) {
        if (elem.type != Type::String) {
            throw ValidationException("Array '" + key + "' must contain only strings.");
        }
        out.push_back(elem.strVal);
    }
    return out;
}

namespace {

void appendUtf8(std::string& out, unsigned int cp) {
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

class Parser {
public:
    explicit Parser(const std::string& src) : src_(src), pos_(0) {}

    JsonValue parseRoot() {
        skipWs();
        JsonValue val = parseValue();
        skipWs();
        if (pos_ != src_.size()) {
            throw ValidationException("Unexpected trailing characters in JSON payload.");
        }
        return val;
    }

private:
    void skipWs() {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
            ++pos_;
        }
    }

    char peek() const {
        if (pos_ >= src_.size()) {
            throw ValidationException("Unexpected end of JSON input.");
        }
        return src_[pos_];
    }

    char get() {
        char c = peek();
        ++pos_;
        return c;
    }

    unsigned int parseHex4() {
        if (pos_ + 4 > src_.size()) {
            throw ValidationException("Incomplete \\u escape in JSON string.");
        }
        unsigned int val = 0;
        for (int i = 0; i < 4; ++i) {
            char c = src_[pos_++];
            val <<= 4;
            if (c >= '0' && c <= '9') val |= static_cast<unsigned int>(c - '0');
            else if (c >= 'a' && c <= 'f') val |= static_cast<unsigned int>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') val |= static_cast<unsigned int>(c - 'A' + 10);
            else throw ValidationException("Invalid hex digit in \\u escape.");
        }
        return val;
    }

    JsonValue parseValue() {
        skipWs();
        char c = peek();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') {
            JsonValue v;
            v.type = JsonValue::Type::String;
            v.strVal = parseString();
            return v;
        }
        if (c == 't' || c == 'f') return parseBool();
        if (c == 'n') return parseNull();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber();
        throw ValidationException("Invalid JSON token encountered.");
    }

    std::string parseString() {
        if (get() != '"') throw ValidationException("Expected '\"' in JSON string.");
        std::string out;
        while (pos_ < src_.size()) {
            char c = get();
            if (c == '"') return out;
            if (c == '\\') {
                char esc = get();
                switch (esc) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        unsigned int cp = parseHex4();
                        appendUtf8(out, cp);
                        break;
                    }
                    default:
                        throw ValidationException("Unsupported escape sequence in JSON string.");
                }
            } else {
                out.push_back(c);
            }
        }
        throw ValidationException("Unterminated JSON string.");
    }

    JsonValue parseNumber() {
        const std::size_t start = pos_;
        bool isFloat = false;
        if (src_[pos_] == '-') ++pos_;
        if (pos_ >= src_.size() || !std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            throw ValidationException("Invalid JSON number.");
        }
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            ++pos_;
        }
        if (pos_ < src_.size() && src_[pos_] == '.') {
            isFloat = true;
            ++pos_;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                ++pos_;
            }
        }
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            isFloat = true;
            ++pos_;
            if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) ++pos_;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                ++pos_;
            }
        }
        JsonValue v;
        const std::string token = src_.substr(start, pos_ - start);
        try {
            if (isFloat) {
                v.type = JsonValue::Type::Double;
                v.doubleVal = std::stod(token);
            } else {
                v.type = JsonValue::Type::Int;
                v.intVal = std::stoll(token);
                v.doubleVal = static_cast<double>(v.intVal);
            }
        } catch (...) {
            throw ValidationException("Numeric literal out of range.");
        }
        return v;
    }

    JsonValue parseBool() {
        if (src_.compare(pos_, 4, "true") == 0) {
            pos_ += 4;
            JsonValue v;
            v.type = JsonValue::Type::Bool;
            v.boolVal = true;
            return v;
        }
        if (src_.compare(pos_, 5, "false") == 0) {
            pos_ += 5;
            JsonValue v;
            v.type = JsonValue::Type::Bool;
            v.boolVal = false;
            return v;
        }
        throw ValidationException("Invalid boolean literal in JSON.");
    }

    JsonValue parseNull() {
        if (src_.compare(pos_, 4, "null") == 0) {
            pos_ += 4;
            return JsonValue{};
        }
        throw ValidationException("Invalid null literal in JSON.");
    }

    JsonValue parseArray() {
        get(); // '['
        JsonValue v;
        v.type = JsonValue::Type::Array;
        skipWs();
        if (peek() == ']') {
            get();
            return v;
        }
        while (true) {
            v.arrVal.push_back(parseValue());
            skipWs();
            char c = get();
            if (c == ']') break;
            if (c != ',') throw ValidationException("Expected ',' or ']' in JSON array.");
        }
        return v;
    }

    JsonValue parseObject() {
        get(); // '{'
        JsonValue v;
        v.type = JsonValue::Type::Object;
        skipWs();
        if (peek() == '}') {
            get();
            return v;
        }
        while (true) {
            skipWs();
            std::string key = parseString();
            skipWs();
            if (get() != ':') throw ValidationException("Expected ':' after JSON object key.");
            v.objVal[std::move(key)] = parseValue();
            skipWs();
            char c = get();
            if (c == '}') break;
            if (c != ',') throw ValidationException("Expected ',' or '}' in JSON object.");
        }
        return v;
    }

    const std::string& src_;
    std::size_t pos_;
};

} // namespace

JsonValue parse(const std::string& src) {
    return Parser(src).parseRoot();
}

std::string escape(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out += ' ';
                } else {
                    out.push_back(c);
                }
                break;
        }
    }
    out.push_back('"');
    return out;
}

std::string intArray(const std::vector<int>& arr) {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) oss << ",";
        oss << arr[i];
    }
    oss << "]";
    return oss.str();
}

std::string stringArray(const std::vector<std::string>& arr) {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) oss << ",";
        oss << escape(arr[i]);
    }
    oss << "]";
    return oss.str();
}

} // namespace dsa::json
