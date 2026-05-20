#pragma once

#include "simulation/topology/Node.hpp"
#include "simulation/metrics/PressureAnalysis.hpp"

#include "raylib.h"

namespace actions_ui {
const char* pressureSeverity(double value);
Color pressureColor(double value);
double overallPressure(const NodePressure* pressure, const Node* node);
}
