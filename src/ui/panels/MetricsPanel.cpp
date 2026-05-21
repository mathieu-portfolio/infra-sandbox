#include "ui/panels/MetricsPanel.hpp"

#include "ui/panels/MetricsPanelRenderer.hpp"
#include "ui/core/UiLayout.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstdio>

namespace {
Rectangle specializationButton(Rectangle bounds, int index)
{
    constexpr float gap = 6.0f;
    constexpr int columns = 3;
    const float buttonWidth = (bounds.width - 28.0f - gap * static_cast<float>(columns - 1)) / static_cast<float>(columns);
    return {
        bounds.x + 14.0f + static_cast<float>(index % columns) * (buttonWidth + gap),
        bounds.y + 38.0f + static_cast<float>(index / columns) * 30.0f,
        buttonWidth,
        24.0f,
    };
}

Rectangle sandboxButton(float x, float y, float width, int index)
{
    return {x + 12.0f + static_cast<float>(index % 2) * ((width - 30.0f) * 0.5f + 6.0f), y + 42.0f + static_cast<float>(index / 2) * 30.0f, (width - 30.0f) * 0.5f, 24.0f};
}
}

void MetricsPanel::update(UiContext& context, const UiFrameView&)
{
    if (context.state == nullptr || !IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }
    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight);
    const LeftSidebarLayout left = computeLeftSidebarLayout(layout.leftSidebar, context.state->sandboxMode);
    const Vector2 mouse = GetMousePosition();

    if (context.state->showMetrics && left.specializations.height > 0.0f) {
        for (int i = 0; i < static_cast<int>(MetricsSpecialization::Count); ++i) {
            if (CheckCollisionPointRec(mouse, specializationButton(left.specializations, i))) {
                context.state->selectedMetricsSpecialization = static_cast<MetricsSpecialization>(i);
                return;
            }
        }
    }

    if (!context.state->sandboxMode) {
        return;
    }
    const float x = left.sandbox.x;
    const float width = left.sandbox.width;
    const float y = left.sandbox.y;
    if (left.sandbox.height <= 0.0f) {
        return;
    }
    const char* requests[] = {"traffic_spike", "retry_storm", "db_slowdown", "regional_traffic_spike", "recovery"};
    for (int i = 0; i < 5; ++i) {
        if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, i))) {
            context.state->sandboxEventRequest = requests[i];
            return;
        }
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 5))) {
        context.state->sandboxQueueBuildup = !context.state->sandboxQueueBuildup;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 6))) {
        context.state->sandboxTrafficMultiplier = std::max(0.1, context.state->sandboxTrafficMultiplier - 0.25);
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 7))) {
        context.state->sandboxTrafficMultiplier = std::min(8.0, context.state->sandboxTrafficMultiplier + 0.25);
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 8))) {
        context.state->sandboxLatencyMultiplier = std::max(0.1, context.state->sandboxLatencyMultiplier - 0.25);
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 9))) {
        context.state->sandboxLatencyMultiplier = std::min(8.0, context.state->sandboxLatencyMultiplier + 0.25);
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 10))) {
        context.state->sandboxResetSimulationRequested = true;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 11))) {
        context.state->sandboxClearTimelineRequested = true;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 12))) {
        context.state->sandboxSlowMotionRequested = true;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 13))) {
        context.state->sandboxStepRequested = true;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 14))) {
        context.state->sandboxSeed = std::max(1, context.state->sandboxSeed - 1);
        context.state->sandboxRegenerateRequested = true;
        return;
    }
    if (CheckCollisionPointRec(mouse, sandboxButton(x, y, width, 15))) {
        context.state->sandboxSeed += 1;
        context.state->sandboxRegenerateRequested = true;
        return;
    }
}

void MetricsPanel::draw(const UiContext& context, const UiFrameView& view) const
{
    MetricsPanelRenderer{}.draw(context, view);
}
