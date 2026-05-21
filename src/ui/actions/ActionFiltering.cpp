#include "ui/actions/ActionFiltering.hpp"

#include <algorithm>

namespace actions_ui {

std::vector<std::string> categoryFilterLabels(const std::vector<ActionCardModel>& cards)
{
    std::vector<std::string> labels{"All"};
    for (const auto& card : cards) {
        if (card.categories.empty()) {
            continue;
        }
        const std::string& category = card.categories.front();
        if (std::find(labels.begin(), labels.end(), category) == labels.end()) {
            labels.push_back(category);
        }
    }
    const bool hasRisky = std::find(labels.begin(), labels.end(), "Risky") != labels.end();
    while (labels.size() < 4) {
        static const char* fallback[] = {"Scaling", "Optimization", "Reliability"};
        const std::string candidate = fallback[std::min(static_cast<int>(labels.size()) - 1, 2)];
        if (std::find(labels.begin(), labels.end(), candidate) == labels.end()) {
            labels.push_back(candidate);
        } else {
            break;
        }
    }
    if (labels.size() > 4) {
        labels.resize(4);
        if (hasRisky && std::find(labels.begin(), labels.end(), "Risky") == labels.end()) {
            labels.back() = "Risky";
        }
    }
    return labels;
}

std::string categoryFilterLabel(const std::vector<ActionCardModel>& cards, int index)
{
    const auto labels = categoryFilterLabels(cards);
    if (index >= 0 && index < static_cast<int>(labels.size())) {
        return labels[static_cast<std::size_t>(index)];
    }
    return "All";
}

bool actionMatchesCategory(const ActionCardModel& card, const std::vector<std::string>& labels, int index)
{
    if (index <= 0 || index >= static_cast<int>(labels.size())) {
        return true;
    }
    const std::string& selected = labels[static_cast<std::size_t>(index)];
    return std::find(card.categories.begin(), card.categories.end(), selected) != card.categories.end();
}

}
