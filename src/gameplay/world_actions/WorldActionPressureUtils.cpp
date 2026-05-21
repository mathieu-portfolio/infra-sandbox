#include "gameplay/world_actions/WorldActionPressureUtils.hpp"

#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace gameplay::world_actions {
namespace {
std::string signedPressurePart(const char* label, double value, bool lowerIsBetter)
{
    if (std::abs(value) < 0.001) {
        return {};
    }
    const bool beneficial = lowerIsBetter ? value < 0.0 : value > 0.0;
    std::string text = beneficial ? "eases " : "raises ";
    text += label;
    return text;
}
}

PressureState scaledPressureEffect(PressureState effect, double intensity)
{
    auto scale = [intensity](double value) {
        return value * intensity;
    };
    effect.frontend.assetWeight = scale(effect.frontend.assetWeight);
    effect.frontend.renderComplexity = scale(effect.frontend.renderComplexity);
    effect.frontend.cacheEfficiency = scale(effect.frontend.cacheEfficiency);
    effect.frontend.realtimeIntensity = scale(effect.frontend.realtimeIntensity);
    effect.frontend.sessionPersistence = scale(effect.frontend.sessionPersistence);
    effect.frontend.mobileCompatibility = scale(effect.frontend.mobileCompatibility);
    effect.backend.requestLoad = scale(effect.backend.requestLoad);
    effect.backend.queuePressure = scale(effect.backend.queuePressure);
    effect.backend.computeIntensity = scale(effect.backend.computeIntensity);
    effect.backend.serviceFragmentation = scale(effect.backend.serviceFragmentation);
    effect.network.bandwidthPressure = scale(effect.network.bandwidthPressure);
    effect.network.latencySensitivity = scale(effect.network.latencySensitivity);
    effect.network.trafficBurstiness = scale(effect.network.trafficBurstiness);
    return effect;
}

bool hasPressureEffect(const PressureState& effect)
{
    return effect.frontend.assetWeight != 0.0
        || effect.frontend.renderComplexity != 0.0
        || effect.frontend.cacheEfficiency != 0.0
        || effect.frontend.realtimeIntensity != 0.0
        || effect.frontend.sessionPersistence != 0.0
        || effect.frontend.mobileCompatibility != 0.0
        || effect.backend.requestLoad != 0.0
        || effect.backend.queuePressure != 0.0
        || effect.backend.computeIntensity != 0.0
        || effect.backend.serviceFragmentation != 0.0
        || effect.network.bandwidthPressure != 0.0
        || effect.network.latencySensitivity != 0.0
        || effect.network.trafficBurstiness != 0.0;
}

std::string pressurePreviewText(const PressureState& effect)
{
    std::vector<std::string> parts;
    auto add = [&parts](std::string part) {
        if (!part.empty()) {
            parts.push_back(std::move(part));
        }
    };
    add(signedPressurePart("asset weight", effect.frontend.assetWeight, true));
    add(signedPressurePart("render complexity", effect.frontend.renderComplexity, true));
    add(signedPressurePart("cache efficiency", effect.frontend.cacheEfficiency, false));
    add(signedPressurePart("realtime intensity", effect.frontend.realtimeIntensity, true));
    add(signedPressurePart("session persistence", effect.frontend.sessionPersistence, false));
    add(signedPressurePart("backend load", effect.backend.requestLoad, true));
    add(signedPressurePart("queue pressure", effect.backend.queuePressure, true));
    add(signedPressurePart("service fragmentation", effect.backend.serviceFragmentation, true));
    add(signedPressurePart("bandwidth pressure", effect.network.bandwidthPressure, true));
    add(signedPressurePart("latency sensitivity", effect.network.latencySensitivity, true));
    add(signedPressurePart("burstiness", effect.network.trafficBurstiness, true));
    if (parts.empty()) {
        return {};
    }

    std::string text = "Pressure effect: " + parts.front();
    for (std::size_t i = 1; i < parts.size() && i < 3; ++i) {
        text += ", ";
        text += parts[i];
    }
    return text;
}

}
