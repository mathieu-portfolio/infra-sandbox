#include "simulation/Node.hpp"

bool Node::isProcessor() const
{
    return type == NodeType::Service || type == NodeType::Database;
}
