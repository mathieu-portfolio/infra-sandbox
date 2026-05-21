#pragma once

#include "content/ContentRegistry.hpp"
#include "content/loading/ContentJsonAccess.hpp"
#include "core/parsing/Json.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace content::loading {

inline std::vector<Json> loadDirectoryObjects(const std::filesystem::path& directory, ContentLoadResult& result)
{
    std::vector<Json> objects;
    if (!std::filesystem::exists(directory)) {
        result.errors.push_back("Missing content folder: " + directory.string());
        return objects;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        Json json = loadJsonFile(entry.path(), result);
        if (json.isArray()) {
            for (const auto& item : json.asArray()) {
                objects.push_back(item);
            }
        } else if (json.isObject()) {
            objects.push_back(json);
        }
    }
    return objects;
}


inline Json substituteTemplateArgs(const Json& value, const Json::Object& args)
{
    if (value.isString()) {
        const std::string& text = value.asString();
        if (text.size() > 1 && text[0] == '$') {
            const auto it = args.find(text.substr(1));
            if (it != args.end()) {
                return it->second;
            }
        }
        return value;
    }
    if (value.isArray()) {
        Json::Array array;
        for (const auto& entry : value.asArray()) {
            array.push_back(substituteTemplateArgs(entry, args));
        }
        return Json(std::move(array));
    }
    if (value.isObject()) {
        Json::Object object;
        for (const auto& [key, entry] : value.asObject()) {
            object[key] = substituteTemplateArgs(entry, args);
        }
        return Json(std::move(object));
    }
    return value;
}

inline Json deepMergeJson(const Json& base, const Json& overrideValue)
{
    if (!base.isObject() || !overrideValue.isObject()) {
        return overrideValue;
    }
    Json::Object merged = base.asObject();
    for (const auto& [key, value] : overrideValue.asObject()) {
        const auto existing = merged.find(key);
        if (existing != merged.end() && existing->second.isObject() && value.isObject()) {
            existing->second = deepMergeJson(existing->second, value);
        } else {
            merged[key] = value;
        }
    }
    return Json(std::move(merged));
}

inline Json resolveTemplateObject(
    const Json& object,
    const std::unordered_map<std::string, Json>& templates,
    ContentLoadResult& result,
    std::vector<std::string>& stack)
{
    if (object.isArray()) {
        Json::Array array;
        for (const auto& entry : object.asArray()) {
            array.push_back(resolveTemplateObject(entry, templates, result, stack));
        }
        return Json(std::move(array));
    }
    if (!object.isObject()) {
        return object;
    }

    Json::Object expandedChildren;
    for (const auto& [key, value] : object.asObject()) {
        if (key == "template" || key == "args" || key == "overrides") {
            expandedChildren[key] = value;
        } else {
            expandedChildren[key] = resolveTemplateObject(value, templates, result, stack);
        }
    }

    const std::string templateId = stringAt(object, "template");
    if (templateId.empty()) {
        return Json(std::move(expandedChildren));
    }

    if (std::find(stack.begin(), stack.end(), templateId) != stack.end()) {
        result.errors.push_back("Template cycle detected while resolving: " + templateId);
        return Json(std::move(expandedChildren));
    }
    const auto templateIt = templates.find(templateId);
    if (templateIt == templates.end()) {
        result.errors.push_back("Missing content template: " + templateId);
        return Json(std::move(expandedChildren));
    }

    stack.push_back(templateId);
    Json resolvedTemplate = resolveTemplateObject(templateIt->second, templates, result, stack);
    stack.pop_back();

    Json::Object args;
    if (const Json* argsJson = object.find("args"); argsJson != nullptr && argsJson->isObject()) {
        for (const auto& [key, value] : argsJson->asObject()) {
            args[key] = resolveTemplateObject(value, templates, result, stack);
        }
    }
    resolvedTemplate = substituteTemplateArgs(resolvedTemplate, args);

    Json::Object localOverrides;
    for (const auto& [key, value] : expandedChildren) {
        if (key != "template" && key != "args" && key != "overrides") {
            localOverrides[key] = value;
        }
    }
    Json merged = deepMergeJson(resolvedTemplate, Json(std::move(localOverrides)));
    if (const Json* overrides = object.find("overrides"); overrides != nullptr && overrides->isObject()) {
        merged = deepMergeJson(merged, resolveTemplateObject(*overrides, templates, result, stack));
    }
    return merged;
}

inline std::vector<Json> resolveTemplateObjects(
    const std::vector<Json>& objects,
    const std::unordered_map<std::string, Json>& templates,
    ContentLoadResult& result)
{
    std::vector<Json> resolved;
    std::vector<std::string> stack;
    for (const auto& object : objects) {
        resolved.push_back(resolveTemplateObject(object, templates, result, stack));
    }
    return resolved;
}

inline std::unordered_map<std::string, Json> loadLayerTemplates(const std::vector<std::filesystem::path>& roots, ContentLoadResult& result)
{
    std::unordered_map<std::string, Json> templates;
    for (const auto& root : roots) {
        const auto directory = root / "templates";
        if (!std::filesystem::exists(directory)) {
            continue;
        }
        for (const auto& object : loadDirectoryObjects(directory, result)) {
            const std::string id = stringAt(object, "id");
            if (id.empty()) {
                result.errors.push_back("Template object in " + directory.string() + " is missing required id.");
                continue;
            }
            templates[id] = object;
        }
    }
    return templates;
}

inline std::vector<Json> loadOptionalDirectoryObjects(const std::filesystem::path& directory, ContentLoadResult& result)
{
    if (!std::filesystem::exists(directory)) {
        return {};
    }
    return loadDirectoryObjects(directory, result);
}

inline std::vector<Json> loadLayerDirectoryObjects(const std::vector<std::filesystem::path>& roots, const std::string& folder, ContentLoadResult& result, bool required = true, const std::unordered_map<std::string, Json>* templates = nullptr)
{
    std::vector<Json> objects;
    bool found = false;
    for (const auto& root : roots) {
        const auto directory = root / folder;
        if (!std::filesystem::exists(directory)) {
            continue;
        }
        found = true;
        auto layerObjects = loadDirectoryObjects(directory, result);
        if (templates != nullptr && !templates->empty()) {
            layerObjects = resolveTemplateObjects(layerObjects, *templates, result);
        }
        objects.insert(objects.end(), layerObjects.begin(), layerObjects.end());
    }
    if (required && !found) {
        result.errors.push_back("Missing content folder in loaded layers: " + folder);
    }
    return objects;
}

} // namespace content::loading
