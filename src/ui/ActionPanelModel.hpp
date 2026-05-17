#pragma once

#include "simulation/Mechanics.hpp"
#include "simulation/Simulation.hpp"
#include "simulation/TopologyMutation.hpp"
#include "ui/UiTypes.hpp"

#include "raylib.h"

#include <string>
#include <vector>

enum class ActionCardKind {
    Mechanic,
    TopologyMutation,
    CancelPreview,
    ConfirmPreview
};

struct ActionCard {
    ActionCardKind kind = ActionCardKind::Mechanic;
    MechanicType mechanic = MechanicType::ScaleUp;
    TopologyMutationType mutation = TopologyMutationType::AddCache;
    std::string name;
    std::string description;
    std::string target;
    std::string helps;
    std::string tradeOff;
    std::vector<std::string> positiveEffects;
    std::vector<std::string> negativeEffects;
    std::vector<std::string> pressureShifts;
    std::vector<std::string> categories;
    std::vector<std::string> usefulWhen;
    std::vector<PressureCategory> affectedPressures;
    std::string architecturalPattern;
    std::string technologyExample;
    std::string unavailableReason;
    std::string stateLabel;
    bool recommended = false;
    double complexityCost = 0.0;
    int currentScaleLevel = 0;
    int maxScaleLevel = 0;
    int regionSlotsUsed = 0;
    int regionSlotLimit = 0;
    int regionSlotUsage = 0;
    bool available = true;
    bool requiresConfirmation = false;
    Rectangle bounds{};
};

class ActionPanelModel {
public:
    [[nodiscard]] std::vector<ActionCard> buildCards(const Simulation& simulation, const UiState& state, int screenWidth, int screenHeight) const;
    [[nodiscard]] Rectangle panelBounds(int screenWidth, int screenHeight) const;
};
