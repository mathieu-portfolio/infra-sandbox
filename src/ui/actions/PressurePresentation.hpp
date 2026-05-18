#pragma once

#include "simulation/Node.hpp"
#include "simulation/PressureAnalysis.hpp"

#include "raylib.h"

namespace actions_ui {
const char* pressureSeverity(double value);
Color pressureColor(double value);
double overallPressure(const NodePressure* pressure, const Node* node);
}
