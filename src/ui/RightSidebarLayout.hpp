#pragma once

#include "raylib.h"

struct RightSidebarLayout {
    Rectangle root{};
    Rectangle header{};
    Rectangle tabs{};
    Rectangle overview{};
    Rectangle actions{};
    Rectangle actionHeader{};
    Rectangle actionList{};
    Rectangle message{};
    Rectangle statusMessage{};
    Rectangle summaryMessage{};
    Rectangle locked{};
    Rectangle button{};
};

[[nodiscard]] RightSidebarLayout computeRightSidebarLayout(Rectangle sidebar);
