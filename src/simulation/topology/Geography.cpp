#include "simulation/topology/Geography.hpp"

#include "simulation/topology/Node.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthRadiusKm = 6371.0;

double radians(double degrees)
{
    return degrees * kPi / 180.0;
}
}

const std::array<RegionDefinition, 6>& GeographicRegistry::definitions()
{
    static const std::array<RegionDefinition, 6> definitions{{
        {RegionId::NorthAmerica, "NorthAmerica", {39.5, -98.35, "NorthAmerica"}},
        {RegionId::Europe, "Europe", {50.1, 8.7, "Europe"}},
        {RegionId::AsiaPacific, "AsiaPacific", {1.35, 103.8, "AsiaPacific"}},
        {RegionId::SouthAmerica, "SouthAmerica", {-23.55, -46.63, "SouthAmerica"}},
        {RegionId::Africa, "Africa", {-1.29, 36.82, "Africa"}},
        {RegionId::Oceania, "Oceania", {-33.86, 151.21, "Oceania"}},
    }};
    return definitions;
}

const RegionDefinition& GeographicRegistry::definition(RegionId id)
{
    const auto& entries = definitions();
    const auto it = std::find_if(entries.begin(), entries.end(), [id](const RegionDefinition& definition) {
        return definition.id == id;
    });
    return it != entries.end() ? *it : entries.front();
}

Vec2 MapProjection::projectEquirectangular(const GeoLocation& location)
{
    const double clampedLatitude = std::clamp(location.latitude, -90.0, 90.0);
    const double normalizedLongitude = std::clamp(location.longitude, -180.0, 180.0);
    const float x = static_cast<float>((normalizedLongitude + 180.0) / 360.0) * worldWidth - worldWidth * 0.5f;
    const float y = worldHeight * 0.5f - static_cast<float>((clampedLatitude + 90.0) / 180.0) * worldHeight;
    return {x, y};
}

double MapProjection::greatCircleKilometers(const GeoLocation& a, const GeoLocation& b)
{
    const double latA = radians(a.latitude);
    const double latB = radians(b.latitude);
    const double deltaLat = radians(b.latitude - a.latitude);
    const double deltaLon = radians(b.longitude - a.longitude);

    const double h = std::sin(deltaLat * 0.5) * std::sin(deltaLat * 0.5)
        + std::cos(latA) * std::cos(latB) * std::sin(deltaLon * 0.5) * std::sin(deltaLon * 0.5);
    return 2.0 * kEarthRadiusKm * std::asin(std::min(1.0, std::sqrt(h)));
}

double GeographicSystem::latencySeconds(const GeoLocation& source, const GeoLocation& target, double baseLatencySeconds) const
{
    const double distanceKm = MapProjection::greatCircleKilometers(source, target);
    const double distanceLatency = distanceKm / 9000.0;
    return std::clamp(baseLatencySeconds + distanceLatency, 0.05, 2.4);
}
