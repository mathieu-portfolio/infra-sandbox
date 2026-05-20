#pragma once

#include "simulation/topology/Link.hpp"
#include "simulation/topology/Node.hpp"

#include <optional>
#include <cstdint>
#include <vector>

class InfrastructureGraph {
public:
    int addNode(Node node);
    int addLink(Link link);

    [[nodiscard]] Node* node(int id);
    [[nodiscard]] const Node* node(int id) const;
    [[nodiscard]] Link* link(int id);
    [[nodiscard]] const Link* link(int id) const;
    [[nodiscard]] Link* firstOutgoingLink(int sourceNodeId);
    [[nodiscard]] const Link* firstOutgoingLink(int sourceNodeId) const;
    [[nodiscard]] std::optional<int> firstProcessorNodeId() const;

    [[nodiscard]] std::vector<Node>& nodes();
    [[nodiscard]] const std::vector<Node>& nodes() const;
    [[nodiscard]] std::vector<Link>& links();
    [[nodiscard]] const std::vector<Link>& links() const;
    [[nodiscard]] std::uint64_t topologyRevision() const;
    void markTopologyChanged();

private:
    std::vector<Node> nodes_;
    std::vector<Link> links_;
    std::uint64_t topologyRevision_ = 0;
};
