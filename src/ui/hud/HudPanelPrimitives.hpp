#pragma once

#include "ui/core/UiTypes.hpp"

#include "raylib.h"

#include <string>

Rectangle hudScenarioDroplistBounds(int screenWidth);
Rectangle hudObjectivesDroplistBounds(int screenWidth);
Rectangle hudPhaseButtonBounds(int screenWidth);
Rectangle hudFeedbackBounds(int screenWidth);
Rectangle hudHelpBounds(int screenWidth);
Rectangle hudOptionsButtonBounds(int screenWidth);
Rectangle hudOptionsMenuBounds(int screenWidth, int screenHeight);

bool hudPointInFieldOrMenu(Vector2 point, Rectangle field, Rectangle menu);

void drawHudTopButton(Rectangle bounds, const char* label, bool active);
void drawHudIconButton(Rectangle bounds, const char* iconId, bool active);
void drawHudDroplistField(Rectangle bounds, const std::string& label, const std::string& value, bool open);
void drawHudMenuShell(Rectangle bounds);
void drawHudDetailRow(const std::string& label, const std::string& value, float x, float y, float width);
