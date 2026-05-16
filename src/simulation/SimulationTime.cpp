#include "simulation/SimulationTime.hpp"

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

const TimeState& SimulationTimeSystem::state() const
{
    return state_;
}
