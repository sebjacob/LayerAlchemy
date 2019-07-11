/* 
 * File:   LayerSetConfig.cpp
 * Author: sjacob
 * 
 * Created on November 22, 2018, 6:14 p.m.
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

StrMapType loadConfigToMap(const string& path) {
    YAML::Node config = _loadConfigFromPath(path);
    StrMapType categoryMap = _categoryMapFromConfig(config);
    return categoryMap;
}