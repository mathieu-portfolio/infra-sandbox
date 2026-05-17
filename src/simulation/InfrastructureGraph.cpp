#include "simulation/InfrastructureGraph.hpp"

int InfrastructureGraph::addNode(Node node)
{
    node.id = static_cast<int>(nodes_.size());
    nodes_.push_back(std::move(node));
    markTopologyChanged();
    return nodes_.back().id;
}

int InfrastructureGraph::addLink(Link link)
{
    link.id = static_cast<int>(links_.size());
    links_.push_back(std::move(link));
    markTopologyChanged();
    return links_.back().id;
}

Node* InfrastructureGraph::node(int id)
{
    if (id < 0 || id >= static_cast<int>(nodes_.size())) {
        return nullptr;
    }
    return &nodes_[id];
}

const Node* InfrastructureGraph::node(int id) const
{
    if (id < 0 || id >= static_cast<int>(nodes_.size())) {
        return nullptr;
    }
    return &nodes_[id];
}

Link* InfrastructureGraph::link(int id)
{
    if (id < 0 || id >= static_cast<int>(links_.size())) {
        return nullptr;
    }
    return &links_[id];
}

const Link* InfrastructureGraph::link(int id) const
{
    if (id < 0 || id >= static_cast<int>(links_.size())) {
        return nullptr;
    }
    return &links_[id];
}

Link* InfrastructureGraph::firstOutgoingLink(int sourceNodeId)
{
    for (auto& candidate : links_) {
        if (candidate.enabled && candidate.sourceNodeId == sourceNodeId) {
            return &candidate;
        }
    }
    return nullptr;
}

const Link* InfrastructureGraph::firstOutgoingLink(int sourceNodeId) const
{
    for (const auto& candidate : links_) {
        if (candidate.enabled && candidate.sourceNodeId == sourceNodeId) {
            return &candidate;
        }
    }
    return nullptr;
}

std::optional<int> InfrastructureGraph::firstProcessorNodeId() const
{
    for (const auto& candidate : nodes_) {
        if (candidate.isProcessor()) {
            return candidate.id;
        }
    }
    return std::nullopt;
}

std::vector<Node>& InfrastructureGraph::nodes()
{
    return nodes_;
}

const std::vector<Node>& InfrastructureGraph::nodes() const
{
    return nodes_;
}

std::vector<Link>& InfrastructureGraph::links()
{
    return links_;
}

const std::vector<Link>& InfrastructureGraph::links() const
{
    return links_;
}

std::uint64_t InfrastructureGraph::topologyRevision() const
{
    return topologyRevision_;
}

void InfrastructureGraph::markTopologyChanged()
{
    ++topologyRevision_;
}
