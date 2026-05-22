#include "content/loading/ContentPackManager.hpp"

#include "core/parsing/Json.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace content {
namespace {
std::string readFile(const std::filesystem::path& path, std::string& error)
{
    std::ifstream input(path);
    if (!input) {
        error = "Unable to open " + path.string();
        return {};
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string stringAt(const Json& object, const std::string& key)
{
    if (const Json* value = object.find(key); value != nullptr && value->isString()) {
        return value->asString();
    }
    return {};
}

std::vector<std::string> stringsAt(const Json& object, const std::string& key)
{
    std::vector<std::string> values;
    const Json* array = object.find(key);
    if (array == nullptr || !array->isArray()) {
        return values;
    }
    for (const auto& entry : array->asArray()) {
        if (entry.isString()) {
            values.push_back(entry.asString());
        }
    }
    return values;
}

ContentPackMetadata parseMetadata(const Json& object)
{
    return {
        .id = stringAt(object, "id"),
        .displayName = stringAt(object, "display_name"),
        .description = stringAt(object, "description"),
        .version = stringAt(object, "version"),
        .author = stringAt(object, "author"),
        .defaultScenarioId = stringAt(object, "default_scenario_id"),
        .dependsOn = stringsAt(object, "depends_on"),
    };
}

bool metadataValid(const ContentPackMetadata& metadata)
{
    return !metadata.id.empty() && !metadata.displayName.empty() && !metadata.version.empty();
}

bool hasJsonFile(const std::filesystem::path& directory)
{
    if (!std::filesystem::exists(directory)) {
        return false;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            return true;
        }
    }
    return false;
}

bool selectablePackShape(const std::filesystem::path& packPath, std::vector<std::string>& missing)
{
    if (!hasJsonFile(packPath / "scenarios")) {
        missing.push_back("scenarios");
    }
    if (!hasJsonFile(packPath / "progression")) {
        missing.push_back("progression");
    }
    return missing.empty();
}
} // namespace

ContentLoadResult ContentPackManager::discover(const std::filesystem::path& contentRoot)
{
    ContentLoadResult result;
    packs_.clear();
    contentRoot_ = contentRoot;
    activePackId_.clear();

    const std::filesystem::path packsRoot = contentRoot / "packs";
    if (!std::filesystem::exists(packsRoot)) {
        result.errors.push_back("Missing content packs folder: " + packsRoot.string());
        return result;
    }

    for (const auto& entry : std::filesystem::directory_iterator(packsRoot)) {
        if (!entry.is_directory()) {
            continue;
        }
        const std::filesystem::path packFile = entry.path() / "pack.json";
        if (!std::filesystem::exists(packFile)) {
            continue;
        }

        std::string error;
        const std::string text = readFile(packFile, error);
        if (!error.empty()) {
            result.errors.push_back(error);
            continue;
        }
        const JsonParseResult parsed = parseJson(text);
        if (!parsed.error.empty() || !parsed.value.isObject()) {
            result.errors.push_back(packFile.string() + ": " + (parsed.error.empty() ? "Invalid pack metadata." : parsed.error));
            continue;
        }
        ContentPackMetadata metadata = parseMetadata(parsed.value);
        if (!metadataValid(metadata)) {
            result.errors.push_back(packFile.string() + ": Pack metadata requires id, display_name, and version.");
            continue;
        }
        std::vector<std::string> missingRequiredContent;
        if (!selectablePackShape(entry.path(), missingRequiredContent)) {
            std::string message = packFile.string() + ": Skipping incomplete content pack " + metadata.id + "; missing ";
            for (std::size_t i = 0; i < missingRequiredContent.size(); ++i) {
                if (i > 0) {
                    message += ", ";
                }
                message += missingRequiredContent[i];
            }
            message += ".";
            result.errors.push_back(std::move(message));
            continue;
        }
        packs_.push_back({std::move(metadata), entry.path()});
    }

    std::stable_sort(packs_.begin(), packs_.end(), [](const ContentPackInfo& lhs, const ContentPackInfo& rhs) {
        return lhs.metadata.displayName < rhs.metadata.displayName;
    });

    result.loaded = !packs_.empty();
    if (packs_.empty() && result.errors.empty()) {
        result.errors.push_back("No content packs found under " + packsRoot.string());
    }
    return result;
}

ContentLoadResult ContentPackManager::discoverDefaultLocations()
{
    std::vector<std::filesystem::path> candidates;
#ifdef INFRA_CONTENT_DIR
    candidates.emplace_back(INFRA_CONTENT_DIR);
#endif
    candidates.emplace_back("content");
    candidates.emplace_back("../content");
    candidates.emplace_back("../../content");
    candidates.emplace_back("../../../content");

    ContentLoadResult lastResult;
    for (const auto& candidate : candidates) {
        if (!std::filesystem::exists(candidate / "packs")) {
            continue;
        }
        ContentLoadResult result = discover(candidate);
        if (result.loaded) {
            return result;
        }
        lastResult = result;
    }
    if (lastResult.errors.empty()) {
        lastResult.errors.push_back("No content packs folder found.");
    }
    return lastResult;
}

ContentLoadResult ContentPackManager::loadPack(const std::string& packId)
{
    const ContentPackInfo* pack = packById(packId);
    if (pack == nullptr) {
        ContentLoadResult result;
        result.errors.push_back("Unknown content pack: " + packId);
        return result;
    }

    std::vector<std::filesystem::path> layers;
    for (const auto& dependency : pack->metadata.dependsOn) {
        if (dependency == "common") {
            layers.push_back(contentRoot_ / "common");
            continue;
        }
        if (const ContentPackInfo* dependencyPack = packById(dependency); dependencyPack != nullptr) {
            layers.push_back(dependencyPack->path);
            continue;
        }
        ContentLoadResult result;
        result.errors.push_back("Content pack " + packId + " has unknown dependency: " + dependency);
        return result;
    }
    layers.push_back(pack->path);

    ContentLoadResult result = ContentRegistry::instance().loadFromLayers(layers);
    if (result.loaded) {
        activePackId_ = pack->metadata.id;
    }
    return result;
}

const std::vector<ContentPackInfo>& ContentPackManager::packs() const
{
    return packs_;
}

const ContentPackInfo* ContentPackManager::packById(const std::string& packId) const
{
    const auto it = std::find_if(packs_.begin(), packs_.end(), [&](const ContentPackInfo& pack) {
        return pack.metadata.id == packId;
    });
    return it != packs_.end() ? &*it : nullptr;
}

const ContentPackInfo* ContentPackManager::activePack() const
{
    return packById(activePackId_);
}

const std::string& ContentPackManager::activePackId() const
{
    return activePackId_;
}

} // namespace content
