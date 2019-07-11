/* 
 * File:   LayerSetConfig.h
 * Author: sjacob
 *
 * Created on November 22, 2018, 6:14 p.m.
 */

#pragma once
#include <yaml-cpp/yaml.h>
#include "LayerSetTypes.h"

// simple function to load yaml or json from a file path to a YAML::Node object
YAML::Node _loadConfigFromPath(const string&);
// converts YAML::Node  data to a map of strings (StrMapType)
StrMapType _categoryMapFromConfig(YAML::Node&);
// simple wrapper function to load a yaml or json file to a map of strings (StrMapType)
StrMapType loadConfigToMap(const string&);
