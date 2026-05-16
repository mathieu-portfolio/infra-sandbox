#include "simulation/Node.hpp"

bool Node::isProcessor() const
{
    return NodeRegistry::processesRequests(type);
}
