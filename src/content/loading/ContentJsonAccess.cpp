#include "content/loading/ContentJsonAccess.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace content::loading {

std::string readFile(const std::filesystem::path& path, std::string& error)
{
    std::ifstream input(path);
    if (!input) {
        error = "Unable to open " + path.string();
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

Json loadJsonFile(const std::filesystem::path& path, ContentLoadResult& result)
{
    std::string error;
    const std::string text = readFile(path, error);
    if (!error.empty()) {
        result.errors.push_back(error);
        return Json{};
    }
    auto parsed = parseJson(text);
    if (!parsed.error.empty()) {
        result.errors.push_back(path.string() + ": " + parsed.error);
        return Json{};
    }
    return parsed.value;
}

std::string stringAt(const Json& object, const std::string& key, const std::string& fallback)
{
    if (const Json* value = object.find(key); value != nullptr && value->isString()) {
        return value->asString();
    }
    return fallback;
}

std::vector<std::string> stringsAt(const Json& object, const std::string& key)
{
    std::vector<std::string> values;
    const Json* array = object.find(key);
    if (array == nullptr || !array->isArray()) {
        return values;
    }
    for (const auto& entry : array->asArray()) {
        if (entry.isString()) {
            values.push_back(entry.asString());
        }
    }
    return values;
}

ContentPackMetadata parsePackMetadata(const Json& object)
{
    return {
        .id = stringAt(object, "id"),
        .displayName = stringAt(object, "display_name"),
        .description = stringAt(object, "description"),
        .version = stringAt(object, "version"),
        .author = stringAt(object, "author"),
        .defaultScenarioId = stringAt(object, "default_scenario_id"),
        .dependsOn = stringsAt(object, "depends_on"),
    };
}

double numberAt(const Json& object, const std::string& key, double fallback)
{
    if (const Json* value = object.find(key); value != nullptr && value->isNumber()) {
        return value->asNumber();
    }
    return fallback;
}

bool boolAt(const Json& object, const std::string& key, bool fallback)
{
    if (const Json* value = object.find(key); value != nullptr && value->isBool()) {
        return value->asBool();
    }
    return fallback;
}

std::size_t sizeAt(const Json& object, const std::string& key, std::size_t fallback)
{
    if (const Json* value = object.find(key); value != nullptr && value->isNumber()) {
        return static_cast<std::size_t>(std::max(0.0, value->asNumber()));
    }
    return fallback;
}

std::vector<std::string> stringsAtAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey)
{
    auto values = stringsAt(object, preferredKey);
    if (!values.empty() || object.find(preferredKey) != nullptr) {
        return values;
    }
    return stringsAt(object, legacyKey);
}

double numberAtAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey, double fallback)
{
    if (const Json* value = object.find(preferredKey); value != nullptr && value->isNumber()) {
        return value->asNumber();
    }
    return numberAt(object, legacyKey, fallback);
}

NumericRange rangeAtAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey, double fallback)
{
    if (object.find(preferredKey) != nullptr) {
        return rangeAt(object, preferredKey, fallback);
    }
    return rangeAt(object, legacyKey, fallback);
}

} // namespace content::loading
