#pragma once

#include "content/ContentRegistry.hpp"
#include "core/parsing/Json.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace content::loading {

[[nodiscard]] std::string readFile(const std::filesystem::path& path, std::string& error);
[[nodiscard]] Json loadJsonFile(const std::filesystem::path& path, ContentLoadResult& result);

[[nodiscard]] std::string stringAt(const Json& object, const std::string& key, const std::string& fallback = {});
[[nodiscard]] std::vector<std::string> stringsAt(const Json& object, const std::string& key);
[[nodiscard]] ContentPackMetadata parsePackMetadata(const Json& object);

[[nodiscard]] double numberAt(const Json& object, const std::string& key, double fallback = 0.0);
[[nodiscard]] bool boolAt(const Json& object, const std::string& key, bool fallback = false);
[[nodiscard]] std::size_t sizeAt(const Json& object, const std::string& key, std::size_t fallback);

[[nodiscard]] std::vector<std::string> stringsAtAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey);

template <typename T, typename F>
[[nodiscard]] std::vector<T> mappedStringsAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey, F mapper)
{
    std::vector<T> values;
    for (const auto& id : stringsAtAny(object, preferredKey, legacyKey)) {
        values.push_back(mapper(id));
    }
    return values;
}

[[nodiscard]] double numberAtAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey, double fallback = 0.0);
[[nodiscard]] NumericRange rangeAtAny(const Json& object, const std::string& preferredKey, const std::string& legacyKey, double fallback = 0.0);

} // namespace content::loading
