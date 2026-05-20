#include "content/validation/ValueSpec.hpp"

#include "content/parsing/Json.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

namespace content {
namespace {
double numberAt(const Json& object, const std::string& key, double fallback = 0.0)
{
    if (const Json* value = object.find(key); value != nullptr && value->isNumber()) {
        return value->asNumber();
    }
    return fallback;
}
} // namespace

NumericRange fixedRange(double value)
{
    return {value, value};
}

NumericRange rangeFromJson(const Json& value, double fallback)
{
    if (value.isNumber()) {
        return fixedRange(value.asNumber());
    }
    if (value.isObject()) {
        NumericRange range{fallback, fallback};
        range.min = numberAt(value, "min", range.min);
        range.max = numberAt(value, "max", range.max);
        return range;
    }
    return fixedRange(fallback);
}

NumericRange rangeAt(const Json& object, const std::string& key, double fallback)
{
    if (const Json* value = object.find(key); value != nullptr) {
        return rangeFromJson(*value, fallback);
    }
    return fixedRange(fallback);
}

bool isFixed(const NumericRange& range)
{
    return range.min == range.max;
}

void validateRange(const NumericRange& range, const std::string& label, std::vector<std::string>& errors)
{
    if (range.min > range.max) {
        errors.push_back(label + " has invalid range min > max.");
    }
}

double sampleUnitInterval(std::uint32_t seed, std::string_view key)
{
    const std::string stableKey(key);
    const std::size_t hash = std::hash<std::string>{}(stableKey)
        ^ (static_cast<std::size_t>(seed) * 0x9e3779b97f4a7c15ULL);
    return static_cast<double>(hash % 100000U) / 99999.0;
}

double sampleNumber(std::uint32_t seed, std::string_view key, double minValue, double maxValue)
{
    if (minValue == maxValue) {
        return minValue;
    }
    const double low = std::min(minValue, maxValue);
    const double high = std::max(minValue, maxValue);
    return low + (high - low) * sampleUnitInterval(seed, key);
}

double sampleRange(const NumericRange& range, std::uint32_t seed, std::string_view key)
{
    return sampleNumber(seed, key, range.min, range.max);
}

int sampleRangeInt(const NumericRange& range, std::uint32_t seed, std::string_view key)
{
    return static_cast<int>(std::round(sampleRange(range, seed, key)));
}

} // namespace content
