#include "content/Json.hpp"

#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace content {
namespace {
const Json kNull{};
const std::string kEmptyString;
const Json::Array kEmptyArray;
const Json::Object kEmptyObject;

class Parser {
public:
    explicit Parser(const std::string& text)
        : text_(text)
    {
    }

    JsonParseResult parse()
    {
        skipWhitespace();
        Json value = parseValue();
        skipWhitespace();
        if (error_.empty() && position_ != text_.size()) {
            fail("Unexpected trailing data.");
        }
        return {std::move(value), error_};
    }

private:
    Json parseValue()
    {
        skipWhitespace();
        if (position_ >= text_.size()) {
            fail("Unexpected end of JSON.");
            return Json{};
        }
        const char c = text_[position_];
        if (c == '"') {
            return Json(parseString());
        }
        if (c == '{') {
            return parseObject();
        }
        if (c == '[') {
            return parseArray();
        }
        if (c == 't') {
            return parseLiteral("true", Json(true));
        }
        if (c == 'f') {
            return parseLiteral("false", Json(false));
        }
        if (c == 'n') {
            return parseLiteral("null", Json(nullptr));
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return parseNumber();
        }
        fail("Unexpected JSON token.");
        return Json{};
    }

    Json parseObject()
    {
        Json::Object object;
        consume('{');
        skipWhitespace();
        if (peek('}')) {
            consume('}');
            return Json(std::move(object));
        }
        while (error_.empty()) {
            skipWhitespace();
            if (!peek('"')) {
                fail("Expected object key.");
                break;
            }
            std::string key = parseString();
            skipWhitespace();
            if (!consume(':')) {
                fail("Expected ':' after object key.");
                break;
            }
            object[std::move(key)] = parseValue();
            skipWhitespace();
            if (peek('}')) {
                consume('}');
                break;
            }
            if (!consume(',')) {
                fail("Expected ',' between object entries.");
                break;
            }
        }
        return Json(std::move(object));
    }

    Json parseArray()
    {
        Json::Array array;
        consume('[');
        skipWhitespace();
        if (peek(']')) {
            consume(']');
            return Json(std::move(array));
        }
        while (error_.empty()) {
            array.push_back(parseValue());
            skipWhitespace();
            if (peek(']')) {
                consume(']');
                break;
            }
            if (!consume(',')) {
                fail("Expected ',' between array items.");
                break;
            }
        }
        return Json(std::move(array));
    }

    std::string parseString()
    {
        std::string value;
        consume('"');
        while (position_ < text_.size()) {
            const char c = text_[position_++];
            if (c == '"') {
                return value;
            }
            if (c == '\\') {
                if (position_ >= text_.size()) {
                    fail("Unterminated string escape.");
                    return value;
                }
                const char escaped = text_[position_++];
                switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    value.push_back(escaped);
                    break;
                case 'n':
                    value.push_back('\n');
                    break;
                case 'r':
                    value.push_back('\r');
                    break;
                case 't':
                    value.push_back('\t');
                    break;
                default:
                    fail("Unsupported string escape.");
                    return value;
                }
            } else {
                value.push_back(c);
            }
        }
        fail("Unterminated string.");
        return value;
    }

    Json parseNumber()
    {
        const char* start = text_.c_str() + position_;
        char* end = nullptr;
        const double value = std::strtod(start, &end);
        if (end == start) {
            fail("Invalid number.");
            return Json(0.0);
        }
        position_ = static_cast<std::size_t>(end - text_.c_str());
        return Json(value);
    }

    Json parseLiteral(const char* literal, Json value)
    {
        const std::string token(literal);
        if (text_.compare(position_, token.size(), token) != 0) {
            fail("Invalid literal.");
            return Json{};
        }
        position_ += token.size();
        return value;
    }

    bool consume(char expected)
    {
        if (!peek(expected)) {
            return false;
        }
        ++position_;
        return true;
    }

    bool peek(char expected) const
    {
        return position_ < text_.size() && text_[position_] == expected;
    }

    void skipWhitespace()
    {
        while (position_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[position_]))) {
            ++position_;
        }
    }

    void fail(const std::string& message)
    {
        if (error_.empty()) {
            error_ = message + " at byte " + std::to_string(position_);
        }
    }

    const std::string& text_;
    std::size_t position_ = 0;
    std::string error_;
};
}

Json::Json(std::nullptr_t)
    : value_(nullptr)
{
}

Json::Json(bool value)
    : value_(value)
{
}

Json::Json(double value)
    : value_(value)
{
}

Json::Json(std::string value)
    : value_(std::move(value))
{
}

Json::Json(Array value)
    : value_(std::move(value))
{
}

Json::Json(Object value)
    : value_(std::move(value))
{
}

bool Json::isNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Json::isBool() const { return std::holds_alternative<bool>(value_); }
bool Json::isNumber() const { return std::holds_alternative<double>(value_); }
bool Json::isString() const { return std::holds_alternative<std::string>(value_); }
bool Json::isArray() const { return std::holds_alternative<Array>(value_); }
bool Json::isObject() const { return std::holds_alternative<Object>(value_); }

bool Json::asBool(bool fallback) const { return isBool() ? std::get<bool>(value_) : fallback; }
double Json::asNumber(double fallback) const { return isNumber() ? std::get<double>(value_) : fallback; }
const std::string& Json::asString() const { return isString() ? std::get<std::string>(value_) : kEmptyString; }
const Json::Array& Json::asArray() const { return isArray() ? std::get<Array>(value_) : kEmptyArray; }
const Json::Object& Json::asObject() const { return isObject() ? std::get<Object>(value_) : kEmptyObject; }

const Json* Json::find(const std::string& key) const
{
    if (!isObject()) {
        return nullptr;
    }
    const auto& object = asObject();
    const auto it = object.find(key);
    return it != object.end() ? &it->second : nullptr;
}

const Json& Json::at(const std::string& key) const
{
    if (const Json* value = find(key)) {
        return *value;
    }
    return kNull;
}

JsonParseResult parseJson(const std::string& text)
{
    return Parser(text).parse();
}

} // namespace content
