#include "simulation/core/SimulationTime.hpp"

#include <algorithm>

void SimulationTimeSystem::reset()
{
    state_ = {};
    state_.speed = 1.0;
}

void SimulationTimeSystem::update(double dt)
{
    state_.elapsedSeconds += dt;
    state_.scenarioElapsedSeconds += dt;
    state_.phaseElapsedSeconds += dt;
    state_.calendarElapsedDays += dt / 86400.0;
    updateCalendarFields();
}

void SimulationTimeSystem::setSpeed(double speed)
{
    state_.speed = std::max(0.0, speed);
}

void SimulationTimeSystem::setPaused(bool paused)
{
    state_.paused = paused;
}

void SimulationTimeSystem::setScenarioElapsed(double seconds)
{
    state_.scenarioElapsedSeconds = std::max(0.0, seconds);
}

void SimulationTimeSystem::setPhaseElapsed(double seconds)
{
    state_.phaseElapsedSeconds = std::max(0.0, seconds);
}

void SimulationTimeSystem::setCalendarElapsedDays(double days)
{
    state_.calendarElapsedDays = std::max(0.0, days);
    updateCalendarFields();
}

const TimeState& SimulationTimeSystem::state() const
{
    return state_;
}

void SimulationTimeSystem::updateCalendarFields()
{
    constexpr int kStartYear = 2026;
    constexpr int kDaysPerMonth = 30;
    constexpr int kMonthsPerYear = 12;
    constexpr int kMinutesPerDay = 24 * 60;
    const int wholeDays = static_cast<int>(state_.calendarElapsedDays);
    const double fractionalDay = state_.calendarElapsedDays - static_cast<double>(wholeDays);
    const int totalMonths = wholeDays / kDaysPerMonth;
    state_.year = kStartYear + totalMonths / kMonthsPerYear;
    state_.month = totalMonths % kMonthsPerYear + 1;
    state_.day = wholeDays % kDaysPerMonth + 1;
    const int minutes = std::clamp(static_cast<int>(fractionalDay * kMinutesPerDay) + 9 * 60, 0, kMinutesPerDay - 1);
    state_.hour = minutes / 60;
    state_.minute = minutes % 60;
}
