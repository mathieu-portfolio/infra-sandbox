#pragma once

#include "raylib.h"

#include <array>
#include <algorithm>

namespace lab_panel {

enum class Action {
    TrafficSpike,
    RetryStorm,
    DbSlowdown,
    RegionalSpike,
    TrafficDown,
    TrafficUp,
    LatencyDown,
    LatencyUp,
    Recovery,
    QueueToggle,
    ResetSim,
    ClearLog,
    SlowMo,
    Step,
    SeedDown,
    SeedUp,
    Count
};

struct ButtonLayout {
    Action action = Action::TrafficSpike;
    Rectangle bounds{};
};

constexpr float headerHeight = 82.0f;
constexpr float viewportTop = headerHeight;
constexpr float contentHeight = 482.0f;
constexpr float panelPadding = 12.0f;
constexpr float sectionGap = 10.0f;
constexpr float sectionHeaderHeight = 22.0f;
constexpr float sectionPadding = 10.0f;
constexpr float buttonHeight = 26.0f;
constexpr float rowGap = 6.0f;
constexpr float columnGap = 8.0f;

inline Rectangle viewport(Rectangle panel)
{
    return {panel.x, panel.y + viewportTop, panel.width, std::max(0.0f, panel.height - viewportTop - 2.0f)};
}

inline Rectangle contentBounds(Rectangle panel, float scrollOffset)
{
    const Rectangle view = viewport(panel);
    return {view.x + panelPadding, view.y + panelPadding - scrollOffset, view.width - panelPadding * 2.0f, contentHeight};
}

inline Rectangle section(Rectangle content, float y, float height)
{
    return {content.x, content.y + y, content.width, height};
}

inline float sectionBodyY(Rectangle sectionBounds)
{
    return sectionBounds.y + sectionPadding + sectionHeaderHeight;
}

inline Rectangle gridButton(Rectangle sectionBounds, int column, int row)
{
    const float usableWidth = sectionBounds.width - sectionPadding * 2.0f;
    const float columnWidth = (usableWidth - columnGap) * 0.5f;
    return {
        sectionBounds.x + sectionPadding + static_cast<float>(column) * (columnWidth + columnGap),
        sectionBodyY(sectionBounds) + static_cast<float>(row) * (buttonHeight + rowGap),
        columnWidth,
        buttonHeight,
    };
}

inline Rectangle fullButton(Rectangle sectionBounds, int row)
{
    return {
        sectionBounds.x + sectionPadding,
        sectionBodyY(sectionBounds) + static_cast<float>(row) * (buttonHeight + rowGap),
        sectionBounds.width - sectionPadding * 2.0f,
        buttonHeight,
    };
}

inline Rectangle seedButton(Rectangle sectionBounds, int column)
{
    const float usableWidth = sectionBounds.width - sectionPadding * 2.0f;
    const float columnWidth = (usableWidth - columnGap * 2.0f) / 3.0f;
    return {
        sectionBounds.x + sectionPadding + static_cast<float>(column) * (columnWidth + columnGap),
        sectionBodyY(sectionBounds),
        columnWidth,
        buttonHeight,
    };
}

inline std::array<ButtonLayout, static_cast<int>(Action::Count)> buttons(Rectangle panel, float scrollOffset)
{
    const Rectangle content = contentBounds(panel, scrollOffset);
    const Rectangle traffic = section(content, 0.0f, 154.0f);
    const Rectangle recovery = section(content, 154.0f + sectionGap, 90.0f);
    const Rectangle sim = section(content, 154.0f + sectionGap + 90.0f + sectionGap, 58.0f);
    const Rectangle seed = section(content, 154.0f + sectionGap + 90.0f + sectionGap + 58.0f + sectionGap, 78.0f);

    return {
        ButtonLayout{Action::TrafficSpike, gridButton(traffic, 0, 0)},
        ButtonLayout{Action::RetryStorm, gridButton(traffic, 1, 0)},
        ButtonLayout{Action::DbSlowdown, gridButton(traffic, 0, 1)},
        ButtonLayout{Action::RegionalSpike, gridButton(traffic, 1, 1)},
        ButtonLayout{Action::TrafficDown, gridButton(traffic, 0, 2)},
        ButtonLayout{Action::TrafficUp, gridButton(traffic, 1, 2)},
        ButtonLayout{Action::LatencyDown, gridButton(traffic, 0, 3)},
        ButtonLayout{Action::LatencyUp, gridButton(traffic, 1, 3)},
        ButtonLayout{Action::Recovery, gridButton(recovery, 0, 0)},
        ButtonLayout{Action::QueueToggle, gridButton(recovery, 1, 0)},
        ButtonLayout{Action::ResetSim, gridButton(recovery, 0, 1)},
        ButtonLayout{Action::ClearLog, gridButton(recovery, 1, 1)},
        ButtonLayout{Action::SlowMo, gridButton(sim, 0, 0)},
        ButtonLayout{Action::Step, gridButton(sim, 1, 0)},
        ButtonLayout{Action::SeedDown, seedButton(seed, 0)},
        ButtonLayout{Action::SeedUp, seedButton(seed, 2)},
    };
}

inline Rectangle seedValueBounds(Rectangle panel, float scrollOffset)
{
    const Rectangle content = contentBounds(panel, scrollOffset);
    const Rectangle seed = section(content, 154.0f + sectionGap + 90.0f + sectionGap + 58.0f + sectionGap, 78.0f);
    return seedButton(seed, 1);
}

inline float maxScroll(Rectangle panel)
{
    const Rectangle view = viewport(panel);
    return std::max(0.0f, contentHeight + panelPadding * 2.0f - view.height);
}

} // namespace lab_panel
