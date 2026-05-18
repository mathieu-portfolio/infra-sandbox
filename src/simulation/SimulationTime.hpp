#pragma once

struct TimeState {
    double elapsedSeconds = 0.0;
    double scenarioElapsedSeconds = 0.0;
    double phaseElapsedSeconds = 0.0;
    double calendarElapsedDays = 0.0;
    int minute = 0;
    int hour = 9;
    int day = 1;
    int month = 1;
    int year = 2026;
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
    void setCalendarElapsedDays(double days);

    [[nodiscard]] const TimeState& state() const;

private:
    void updateCalendarFields();

    TimeState state_{};
};
