#pragma once

#include "gameplay/scenario/data/ScenarioEnums.hpp"

struct EngineeringCapacity {
    int frontend = 1;
    int backend = 2;
    int infrastructure = 1;
    int data = 1;
    int operations = 1;
    int total = 8;
};

struct EngineeringCost {
    EngineeringDomain domain = EngineeringDomain::Backend;
    int amount = 0;
};
