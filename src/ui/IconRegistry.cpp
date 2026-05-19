#include "ui/IconRegistry.hpp"

#include "content/ContentRegistry.hpp"
#include "content/Json.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace {
std::string readTextFile(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string stringAt(const content::Json& object, const std::string& key, const std::string& fallback = {})
{
    if (const content::Json* value = object.find(key); value != nullptr && value->isString()) {
        return value->asString();
    }
    return fallback;
}
}

IconRegistry& IconRegistry::instance()
{
    static IconRegistry registry;
    return registry;
}

bool IconRegistry::hasIcon(const std::string& id)
{
    return entryFor(id).loaded;
}

bool IconRegistry::drawIcon(const std::string& id, Rectangle bounds, Color tint)
{
    auto& entry = entryFor(id);
    if (!entry.loaded) {
        drawFallback(resolveIconId(id), bounds, tint);
        return false;
    }

    DrawTexturePro(
        entry.texture,
        {0.0f, 0.0f, static_cast<float>(entry.texture.width), static_cast<float>(entry.texture.height)},
        bounds,
        {0.0f, 0.0f},
        0.0f,
        tint);
    return true;
}

void IconRegistry::drawFallback(const std::string& id, Rectangle bounds, Color tint) const
{
    const Vector2 center{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};
    const float radius = std::min(bounds.width, bounds.height) * 0.36f;
    DrawCircleV(center, radius + 4.0f, {tint.r, tint.g, tint.b, 34});

    if (id.find("database") != std::string::npos || id.find("persistence") != std::string::npos) {
        DrawRectangleRounded({center.x - radius, center.y - radius * 0.75f, radius * 2.0f, radius * 1.5f}, 0.2f, 6, {tint.r, tint.g, tint.b, 80});
        DrawRectangleRoundedLines({center.x - radius, center.y - radius * 0.75f, radius * 2.0f, radius * 1.5f}, 0.2f, 6, tint);
    } else if (id.find("cache") != std::string::npos) {
        DrawPoly(center, 6, radius, 30.0f, {tint.r, tint.g, tint.b, 80});
        DrawPolyLines(center, 6, radius, 30.0f, tint);
    } else if (id.find("alert") != std::string::npos || id.find("warning") != std::string::npos) {
        DrawTriangle({center.x, center.y - radius}, {center.x + radius, center.y + radius}, {center.x - radius, center.y + radius}, {tint.r, tint.g, tint.b, 80});
        DrawTriangleLines({center.x, center.y - radius}, {center.x + radius, center.y + radius}, {center.x - radius, center.y + radius}, tint);
    } else if (id.find("action") != std::string::npos) {
        DrawRectangleRounded({center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f}, 0.25f, 6, {tint.r, tint.g, tint.b, 75});
        DrawRectangleRoundedLines({center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f}, 0.25f, 6, tint);
    } else {
        DrawCircleV(center, radius, {tint.r, tint.g, tint.b, 75});
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, tint);
    }
}

void IconRegistry::release()
{
    for (auto& [id, entry] : icons_) {
        (void)id;
        if (entry.loaded) {
            UnloadTexture(entry.texture);
        }
    }
    icons_.clear();
}

IconRegistry::IconEntry& IconRegistry::entryFor(const std::string& id)
{
    ensureConfigLoaded();

    const std::string resolvedId = resolveIconId(id);
    auto& entry = icons_[resolvedId];
    if (!entry.attempted) {
        entry.attempted = true;
        const std::string path = pathFor(resolvedId);
        if (FileExists(path.c_str())) {
            entry.texture = LoadTexture(path.c_str());
            entry.loaded = entry.texture.id > 0;
        }
    }
    return entry;
}

std::string IconRegistry::pathFor(const std::string& id) const
{
    if (const auto it = iconPaths_.find(id); it != iconPaths_.end()) {
        return it->second;
    }

    std::string file = id;
    std::replace(file.begin(), file.end(), '.', '_');
    return "assets/icons/" + file + ".png";
}

std::string IconRegistry::resolveIconId(const std::string& id) const
{
    if (const auto it = aliases_.find(id); it != aliases_.end()) {
        return it->second;
    }
    return id;
}

void IconRegistry::ensureConfigLoaded()
{
    if (configLoaded_) {
        return;
    }
    configLoaded_ = true;
    const auto& packPath = content::ContentRegistry::instance().currentPackPath();
    if (!packPath.empty()) {
        loadConfig((packPath / "ui" / "icons.json").string());
    }
}

void IconRegistry::loadConfig(const std::string& path)
{
    const std::string text = readTextFile(path);
    if (text.empty()) {
        return;
    }

    const content::JsonParseResult parsed = content::parseJson(text);
    if (!parsed.error.empty() || !parsed.value.isObject()) {
        return;
    }

    if (const content::Json* icons = parsed.value.find("icons"); icons != nullptr && icons->isObject()) {
        for (const auto& [id, entry] : icons->asObject()) {
            if (entry.isString()) {
                iconPaths_[id] = entry.asString();
            } else if (entry.isObject()) {
                const std::string iconPath = stringAt(entry, "path");
                if (!iconPath.empty()) {
                    iconPaths_[id] = iconPath;
                }
            }
        }
    }

    if (const content::Json* aliases = parsed.value.find("aliases"); aliases != nullptr && aliases->isObject()) {
        for (const auto& [alias, target] : aliases->asObject()) {
            if (target.isString()) {
                aliases_[alias] = target.asString();
            }
        }
    }
}
