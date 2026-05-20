#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace content {
class Json;
}

struct NumericRange {
    double min = 0.0;
    double max = 0.0;
};

namespace content {

[[nodiscard]] NumericRange fixedRange(double value);
[[nodiscard]] NumericRange rangeFromJson(const Json& value, double fallback);
[[nodiscard]] NumericRange rangeAt(const Json& object, const std::string& key, double fallback);

[[nodiscard]] bool isFixed(const NumericRange& range);
void validateRange(const NumericRange& range, const std::string& label, std::vector<std::string>& errors);

[[nodiscard]] double sampleUnitInterval(std::uint32_t seed, std::string_view key);
[[nodiscard]] double sampleRange(const NumericRange& range, std::uint32_t seed, std::string_view key);
[[nodiscard]] int sampleRangeInt(const NumericRange& range, std::uint32_t seed, std::string_view key);
[[nodiscard]] double sampleNumber(std::uint32_t seed, std::string_view key, double minValue, double maxValue);

} // namespace content
