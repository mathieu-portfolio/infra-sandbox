#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace content {

class Json {
public:
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json>;

    Json() = default;
    explicit Json(std::nullptr_t);
    explicit Json(bool value);
    explicit Json(double value);
    explicit Json(std::string value);
    explicit Json(Array value);
    explicit Json(Object value);

    [[nodiscard]] bool isNull() const;
    [[nodiscard]] bool isBool() const;
    [[nodiscard]] bool isNumber() const;
    [[nodiscard]] bool isString() const;
    [[nodiscard]] bool isArray() const;
    [[nodiscard]] bool isObject() const;

    [[nodiscard]] bool asBool(bool fallback = false) const;
    [[nodiscard]] double asNumber(double fallback = 0.0) const;
    [[nodiscard]] const std::string& asString() const;
    [[nodiscard]] const Array& asArray() const;
    [[nodiscard]] const Object& asObject() const;

    [[nodiscard]] const Json* find(const std::string& key) const;
    [[nodiscard]] const Json& at(const std::string& key) const;

private:
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value_{nullptr};
};

struct JsonParseResult {
    Json value;
    std::string error;
};

[[nodiscard]] JsonParseResult parseJson(const std::string& text);

} // namespace content
