/* 
 * implementation code focused on configuration file handling
 */
#include "LayerSetConfig.h"

YAML::Node _loadConfigFromPath(const string& path) {
    YAML::Node config = YAML::LoadFile(path);
    return config;
}

StrMapType _categoryMapFromConfig(YAML::Node& config) {
    StrMapType categoryMap;
    for (YAML::const_iterator it = config.begin(); it != config.end(); it++) {
        string categoryName = it->first.as<string>();
        StrVecType values = it->second.as<StrVecType>();
        categoryMap[categoryName] = values;
        }
    return categoryMap;
}

StrMapType loadConfigToMap(const string& yamlFilePath) {
    YAML::Node config = _loadConfigFromPath(yamlFilePath);
    StrMapType categoryMap = _categoryMapFromConfig(config);
    return categoryMap;
}