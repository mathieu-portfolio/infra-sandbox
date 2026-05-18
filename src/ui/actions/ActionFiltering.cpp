#include "ui/actions/ActionFiltering.hpp"

#include <algorithm>

namespace actions_ui {

std::string categoryFilterLabel(const std::vector<ActionCardModel>& cards, int index)
{
    if (index == 0) return "All";
    std::vector<std::string> categories;
    for (const auto& card : cards) {
        if (!card.available || card.categories.empty()) {
            continue;
        }
        if (std::find(categories.begin(), categories.end(), card.categories.front()) == categories.end()) {
            categories.push_back(card.categories.front());
        }
    }
    if (index - 1 < static_cast<int>(categories.size())) {
        return categories[static_cast<std::size_t>(index - 1)];
    }
    static const char* fallback[] = {"Scaling", "Optimization", "Reliability"};
    return fallback[std::min(index - 1, 2)];
}

}
