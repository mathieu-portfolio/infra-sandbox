#pragma once

struct TimeState {
    double elapsedSeconds = 0.0;
    double scenarioElapsedSeconds = 0.0;
    double phaseElapsedSeconds = 0.0;
    double speed = 1.0;
    bool paused = false;
};

class SimulationTimeSystem {
public:
    void reset();
    void update(double dt);
    void setSpeed(double speed);
    void setPaused(bool paused);
    void setScenarioElapsed(double seconds);
    void setPhaseElapsed(double seconds);

    [[nodiscard]] const TimeState& state() const;

private:
    TimeState state_{};
};
