#pragma once

#include "simulation/requests/Request.hpp"
#include "simulation/topology/Link.hpp"
#include "simulation/topology/Node.hpp"

#include <optional>

class Simulation;

class SimulationRequestFlowSystem {
public:
    static void generateClientRequests(Simulation& simulation, double dt);
    static void updateRetryWaits(Simulation& simulation);
    static void updateLinks(Simulation& simulation, double dt);
    static void updateProcessors(Simulation& simulation, double dt);
    static void expireCacheEntries(Simulation& simulation);
    static void pruneOldRequests(Simulation& simulation);

private:
    static void createRequest(Simulation& simulation, Node& clientNode, Link& link);
    static void retryRequest(Simulation& simulation, Request& request);
    static void updateTimeouts(Simulation& simulation, Node& node);
    static void enqueueAtNode(Simulation& simulation, Request& request, Node& node);
    static void completeRequest(Simulation& simulation, Request& request, Node& node);
    static void timeOutRequest(Simulation& simulation, Request& request, Node& node);
    static void timeOutRequest(Simulation& simulation, Request& request);
    static void routeFromApi(Simulation& simulation, Request& request);
    static void routeFromDatabase(Simulation& simulation, Request& request);
    static void routeToLink(Simulation& simulation, Request& request, Link& link, RequestRouteStage nextStage);
    [[nodiscard]] static Link* selectOutgoingLink(Simulation& simulation, int sourceNodeId, RequestRouteStage routeStage);
    [[nodiscard]] static Link* selectClientIngressLink(Simulation& simulation, const Node& clientNode);
    [[nodiscard]] static double trafficEvolutionMultiplierFor(const Simulation& simulation, const Node& node, double burstMultiplier);
    [[nodiscard]] static double retryEvolutionDelayMultiplierFor(const Simulation& simulation, const Node& node);
    [[nodiscard]] static Link* linkBetween(Simulation& simulation, int sourceNodeId, int targetNodeId);
    [[nodiscard]] static std::optional<int> firstNodeOfType(const Simulation& simulation, NodeType type);
    [[nodiscard]] static double processingCost(const Simulation& simulation, const Request& request, const Node& node);
    [[nodiscard]] static bool requestTimedOut(const Simulation& simulation, const Request& request);
    [[nodiscard]] static bool cacheHit(Simulation& simulation, int key);
    static void storeCache(Simulation& simulation, int key);
};
