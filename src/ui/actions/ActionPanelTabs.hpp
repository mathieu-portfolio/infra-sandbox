#pragma once

#include "ui/core/UiTypes.hpp"

#include "raylib.h"

Rectangle tabBounds(Rectangle bounds, int index, int count);
NodeInspectionTab nodeTabAt(int index);
const char* nodeTabLabel(NodeInspectionTab tab);
bool nodeTabUnlocked(const ObservabilityState& observability, NodeInspectionTab tab);
void drawLockedObservability(Rectangle bounds, NodeInspectionTab tab);
