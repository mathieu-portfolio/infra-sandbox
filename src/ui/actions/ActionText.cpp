#include "ui/actions/ActionText.hpp"

#include "ui/core/UiPrimitives.hpp"

namespace actions_ui {

void drawWrappedTextClipped(const std::string& text, Rectangle bounds, int fontSize, Color color, float lineSpacing)
{
    drawTextWrappedClipped(text, bounds, fontSize, color, lineSpacing);
}

}
