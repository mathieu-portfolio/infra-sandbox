#pragma once

#include <array>
#include <string>
#include <string_view>

struct Vec2;

enum class RegionId {
    NorthAmerica,
    Europe,
    AsiaPacific,
    SouthAmerica,
    Africa,
    Oceania
};

struct GeoLocation {
    double latitude = 0.0;
    double longitude = 0.0;
    std::string regionName;
};

struct NetworkIdentity {
    std::string hostname;
    std::string ipAddress;
    std::string endpoint;
};

struct RegionDefinition {
    RegionId id = RegionId::Europe;
    std::string_view name;
    GeoLocation center;
};

class GeographicRegistry {
public:
    [[nodiscard]] static const std::array<RegionDefinition, 6>& definitions();
    [[nodiscard]] static const RegionDefinition& definition(RegionId id);
};

class MapProjection {
public:
    static constexpr float worldWidth = 1100.0f;
    static constexpr float worldHeight = 550.0f;

    [[nodiscard]] static Vec2 projectEquirectangular(const GeoLocation& location);
    [[nodiscard]] static double greatCircleKilometers(const GeoLocation& a, const GeoLocation& b);
};

class GeographicSystem {
public:
    [[nodiscard]] double latencySeconds(const GeoLocation& source, const GeoLocation& target, double baseLatencySeconds) const;
};
