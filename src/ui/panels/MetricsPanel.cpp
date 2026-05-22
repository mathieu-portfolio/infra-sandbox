#include "ui/panels/MetricsPanel.hpp"

#include "ui/panels/MetricsPanelRenderer.hpp"
#include "ui/panels/LabPanelLayout.hpp"
#include "ui/core/UiLayout.hpp"
#include "ui/core/ScrollHandling.hpp"

#include "raylib.h"

#include <algorithm>
#include <cstdio>

namespace {
Rectangle specializationButton(Rectangle bounds, int index)
{
    constexpr float rowHeight = 28.0f;
    return {
        bounds.x + 14.0f,
        bounds.y + 38.0f + static_cast<float>(index) * rowHeight,
        bounds.width - 28.0f,
        24.0f,
    };
}

}

void MetricsPanel::update(UiContext& context, const UiFrameView& view)
{
    if (context.state == nullptr) {
        return;
    }
    const UiLayout layout = computeUiLayout(context.screenWidth, context.screenHeight, context.state->dockLayout);
    const LeftSidebarLayout left = computeLeftSidebarLayout(layout.leftSidebar, context.state->sandboxMode);
    const Vector2 mouse = GetMousePosition();


    constexpr float alertsHeaderHeight = 32.0f;
    constexpr float alertsTopPadding = 8.0f;
    constexpr float alertsBottomPadding = 10.0f;
    constexpr float alertsRowHeight = 32.0f;
    const int alertRows = static_cast<int>(view.pressure().hints.size() + view.pressure().suspiciousPatterns.size() + view.pressure().explanations.size());
    const Rectangle alertsViewport = ui::scrollViewport(left.alerts, alertsHeaderHeight);
    const float alertsContentHeight = alertsTopPadding + static_cast<float>(std::max(1, alertRows)) * alertsRowHeight + alertsBottomPadding;
    const float wheel = GetMouseWheelMove();
    (void)ui::updateScrollOffset(alertsViewport, alertsContentHeight, wheel, mouse, context.state->alertsScrollOffset);

    if (context.state->sandboxMode && left.sandbox.height > 0.0f) {
        (void)ui::updateScrollOffset(lab_panel::viewport(left.sandbox), lab_panel::contentHeight + lab_panel::panelPadding * 2.0f, wheel, mouse, context.state->sandboxScrollOffset);
        context.state->sandboxScrollOffset = std::clamp(context.state->sandboxScrollOffset, 0.0f, lab_panel::maxScroll(left.sandbox));
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }

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
    if (left.sandbox.height <= 0.0f || !CheckCollisionPointRec(mouse, lab_panel::viewport(left.sandbox))) {
        return;
    }

    for (const auto& button : lab_panel::buttons(left.sandbox, context.state->sandboxScrollOffset)) {
        if (!CheckCollisionPointRec(mouse, button.bounds)) {
            continue;
        }
        switch (button.action) {
        case lab_panel::Action::TrafficSpike:
            context.state->sandboxEventRequest = "traffic_spike";
            return;
        case lab_panel::Action::RetryStorm:
            context.state->sandboxEventRequest = "retry_storm";
            return;
        case lab_panel::Action::DbSlowdown:
            context.state->sandboxEventRequest = "db_slowdown";
            return;
        case lab_panel::Action::RegionalSpike:
            context.state->sandboxEventRequest = "regional_traffic_spike";
            return;
        case lab_panel::Action::Recovery:
            context.state->sandboxEventRequest = "recovery";
            return;
        case lab_panel::Action::QueueToggle:
            context.state->sandboxQueueBuildup = !context.state->sandboxQueueBuildup;
            return;
        case lab_panel::Action::TrafficDown:
            context.state->sandboxTrafficMultiplier = std::max(0.1, context.state->sandboxTrafficMultiplier - 0.25);
            return;
        case lab_panel::Action::TrafficUp:
            context.state->sandboxTrafficMultiplier = std::min(8.0, context.state->sandboxTrafficMultiplier + 0.25);
            return;
        case lab_panel::Action::LatencyDown:
            context.state->sandboxLatencyMultiplier = std::max(0.1, context.state->sandboxLatencyMultiplier - 0.25);
            return;
        case lab_panel::Action::LatencyUp:
            context.state->sandboxLatencyMultiplier = std::min(8.0, context.state->sandboxLatencyMultiplier + 0.25);
            return;
        case lab_panel::Action::ResetSim:
            context.state->sandboxResetSimulationRequested = true;
            return;
        case lab_panel::Action::ClearLog:
            context.state->sandboxClearTimelineRequested = true;
            return;
        case lab_panel::Action::SlowMo:
            context.state->sandboxSlowMotionRequested = true;
            return;
        case lab_panel::Action::Step:
            context.state->sandboxStepRequested = true;
            return;
        case lab_panel::Action::SeedDown:
            context.state->sandboxSeed = std::max(1, context.state->sandboxSeed - 1);
            context.state->sandboxRegenerateRequested = true;
            return;
        case lab_panel::Action::SeedUp:
            context.state->sandboxSeed += 1;
            context.state->sandboxRegenerateRequested = true;
            return;
        case lab_panel::Action::Count:
            return;
        }
    }
}

void MetricsPanel::draw(const UiContext& context, const UiFrameView& view) const
{
    MetricsPanelRenderer{}.draw(context, view);
}
