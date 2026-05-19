#pragma once

#include "content/ContentRegistry.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace content {

struct ContentPackInfo {
    ContentPackMetadata metadata;
    std::filesystem::path path;
};

class ContentPackManager {
public:
    [[nodiscard]] ContentLoadResult discover(const std::filesystem::path& contentRoot);
    [[nodiscard]] ContentLoadResult discoverDefaultLocations();
    [[nodiscard]] ContentLoadResult loadPack(const std::string& packId);

    [[nodiscard]] const std::vector<ContentPackInfo>& packs() const;
    [[nodiscard]] const ContentPackInfo* packById(const std::string& packId) const;
    [[nodiscard]] const ContentPackInfo* activePack() const;
    [[nodiscard]] const std::string& activePackId() const;

private:
    std::vector<ContentPackInfo> packs_;
    std::filesystem::path contentRoot_;
    std::string activePackId_;
};

} // namespace content
