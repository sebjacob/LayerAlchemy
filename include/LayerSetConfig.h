/* 
 * File:   LayerSetConfig.h
 * Author: sjacob
 *
 * Created on November 22, 2018, 6:14 p.m.
 */

#pragma once
#include <yaml-cpp/yaml.h>
#include "LayerSetTypes.h"

//In the channels config files, these values will be used for topology functions
static const string TOPOLOGY_KEY_LEXICAL = "topology";

// simple function to load yaml or json from a file path to a YAML::Node object
YAML::Node _loadConfigFromPath(const string&);
// converts YAML::Node  data to a map of strings (StrMapType)
StrMapType _categoryMapFromConfig(const YAML::Node&);
// simple wrapper function to load a yaml or json file to a map of strings (StrMapType)
StrMapType loadConfigToMap(const string&);
