#pragma once

#include "raylib.h"

#include <string>
#include <unordered_map>

class IconRegistry {
public:
    static IconRegistry& instance();

    bool hasIcon(const std::string& id);
    bool drawIcon(const std::string& id, Rectangle bounds, Color tint);
    void drawFallback(const std::string& id, Rectangle bounds, Color tint) const;
    void release();

private:
    struct IconEntry {
        Texture2D texture{};
        bool attempted = false;
        bool loaded = false;
    };

    IconEntry& entryFor(const std::string& id);
    std::string pathFor(const std::string& id) const;
    std::string resolveIconId(const std::string& id) const;
    void ensureConfigLoaded();
    void loadConfig(const std::string& path);

    std::unordered_map<std::string, IconEntry> icons_;
    std::unordered_map<std::string, std::string> iconPaths_;
    std::unordered_map<std::string, std::string> aliases_;
    bool configLoaded_ = false;
};
