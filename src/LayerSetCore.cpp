/*
 * implementation code for the LayerMap and LayerCollection objects
 */

#include <iostream>
#include <algorithm>

#include "LayerSetCore.h"

LayerMap::LayerMap() {
};

LayerMap::LayerMap(const LayerMap& other)
: strMap(other.strMap) {
}

LayerMap::LayerMap(const StrMapType& other)
: strMap(other) {
}

LayerMap::~LayerMap() {
}

LayerMap::LayerMap(const string& yamlFilePath) : strMap(loadConfigToMap(yamlFilePath)) {
}

StrVecType LayerMap::operator[](const StrVecType& categoryNames) const {
    StrVecType items;
    for (auto it = categoryNames.begin(); it != categoryNames.end(); it++) {
        if (strMap.find(*it) != strMap.end()) {
            StrVecType categories = strMap.at(*it);
            if (categories.size() != 0) {
                for (auto it2 = categories.begin(); it2 != categories.end(); it2++) {
                    items.push_back(*it2);
                }
            }
        }
    }
    return items;
}

StrVecType LayerMap::operator[](const string& categoryName) const {
    if (strMap.find(categoryName) != strMap.end()) {
        return strMap.at(categoryName);
    } else {
        return StrVecType();
    }
}

bool LayerMap::isMember(const string& categoryName, const string& layer) const {
    auto it = strMap.find(categoryName);
    if (it != strMap.end()) {
        return find(begin(it->second), end(it->second), layer) != it->second.end();
    } else {
        return false;
    }
}

void LayerMap::add(const string categoryName, const string layer) {
    if (strMap.count(categoryName)) {
        if (!isMember(categoryName, layer)) {
            strMap[categoryName].emplace_back(layer);
        }
    } else {
        strMap[categoryName].emplace_back(layer);
    }
}

const StrVecType LayerMap::categories() const {
    StrVecType itemKeys;
    itemKeys.reserve(strMap.size());
    for (auto& kvp : strMap) {
        if (kvp.first.find("_") != 0) {
            itemKeys.emplace_back(kvp.first);
        }
    }
    return itemKeys;
}

const StrVecType LayerMap::categories(string& layerName) const {
    StrVecType itemKeys;
    for (auto& kvp : strMap) {
        if (isMember(kvp.first, layerName)) {
            itemKeys.emplace_back(layerName);
        }
    }
    return itemKeys;
}
const StrVecType LayerMap::uniqueLayers() const {
    StrVecType uniqueLayers;
    for (auto & kvp : strMap) {
        for (auto layer : kvp.second) {
            if (!(find(begin(uniqueLayers), end(uniqueLayers), layer) != uniqueLayers.end())) {
                uniqueLayers.push_back(layer);
            }
        }
    }
    return uniqueLayers;
}

string LayerMap::toString() const {
    std::ostringstream reprStream;
    string repr;
    reprStream <<
    "<LayerSetCore.LayerMap object at " << &strMap << "> \n\n{\n";
    for (auto & kvp : strMap) {
        reprStream << "'" << (string) kvp.first << "'" << " : (";
        for (unsigned m = 0; m < kvp.second.size(); m++) {
            reprStream << "'" << (string) kvp.second[m] << "'" << ", ";
        }
        reprStream << "),\n";
    }
    reprStream << "}\n";
    repr = reprStream.str();
    return repr;
}

const StrVecType LayerMap::categoriesByType(const categorizeType& catType) const {
    StrVecType relevantCats;
    relevantCats.reserve(strMap.size());
    for (const auto& kvp : strMap) {
        int firstToken = kvp.first.find("_");
        if ((firstToken == 0) && (catType == categorizeType::priv)) { // looking for categories starting with _ in priv mode
            relevantCats.emplace_back(kvp.first);
        } else {
            if ((firstToken != 0) && (catType == categorizeType::pub)) {
                relevantCats.emplace_back(kvp.first);
            }
        }
    };
    return relevantCats;
};

CategorizeFilter::CategorizeFilter() {
    filterMode = CategorizeFilter::modes::INCLUDE;
}

CategorizeFilter::CategorizeFilter(const StrVecType& selectedCategories, int mode) {
    filterMode = mode;
    categories = selectedCategories;
}

LayerCollection::LayerCollection() :
channels(LayerMap(loadConfigToMap((getenv(CHANNEL_ENV_VAR))))),
layers(LayerMap(loadConfigToMap((getenv(LAYER_ENV_VAR))))) {
}

LayerCollection::~LayerCollection() {
}

string LayerCollection::dePrefix(const string layerName) const {
    string unPrefixedLayerName, outputLayerName;
    StrVecType prefix = layers.strMap.at("_prefix");

    for (StrVecType::const_iterator iterPrefix = prefix.begin(); iterPrefix != prefix.end(); iterPrefix++) {
        string prefixName = *iterPrefix;
        string prefixToFind = prefixName + "_";
        size_t prefixLocation = layerName.rfind(prefixToFind);
        string pos = layerName.substr(0, prefixLocation);
        if ((prefixLocation != string::npos) || (layerName.substr(0, prefixToFind.length()) == prefixToFind)) {
            unPrefixedLayerName = prefixName;
        }
    }
    return (unPrefixedLayerName.length() > 0) ? unPrefixedLayerName : layerName;
}

bool LayerMap::contains(const string& categoryName) const {
    StrVecType categories = this->categories();
    bool result = std::any_of(categories.begin(), categories.end(), [categoryName](const string & str) {
        return str == categoryName; });
    return result;
}

bool LayerMap::contains(const StrVecType& categoryNames) const {
    StrVecType categories = this->categories();
    return std::includes(categories.begin(), categories.end(), categoryNames.begin(), categoryNames.end());
}

string utilities::getLayerFromChannel(const string& layerName) {
    return layerName.substr(0, layerName.find("."));
};

StrVecType utilities::applyChannelNames(const string& layerName, const StrVecType& topologyVector) {
    StrVecType channelNames;
    channelNames.reserve(topologyVector.size());
    for (StrVecType::const_iterator iterChannel = topologyVector.begin(); iterChannel != topologyVector.end(); iterChannel++) {
        channelNames.emplace_back(layerName + "." + *iterChannel);
    }
    return channelNames;
}

LayerMap LayerCollection::categorizeLayers(const StrVecType& layersToCategorize, const categorizeType& catType) const {
    LayerMap categorizedLayerMap;
    StrVecType relevantCats = layers.categoriesByType(catType);

    for (StrVecType::const_iterator iterLayer = layersToCategorize.begin(); iterLayer != layersToCategorize.end(); iterLayer++) {
        string realLayerName;
        string layerName = utilities::getLayerFromChannel(*iterLayer);
        string dePrefixedLayerName = dePrefix(layerName);
        categorizedLayerMap.add("all", layerName);

        for (StrVecType::const_iterator iterCat = relevantCats.begin(); iterCat != relevantCats.end(); iterCat++) {
            if (layers.isMember(*iterCat, dePrefixedLayerName) | layers.isMember(*iterCat, layerName)) {
                categorizedLayerMap.add(*iterCat, layerName);
            }
        }
    }
    return categorizedLayerMap;
};

LayerMap LayerCollection::categorizeLayers(const StrVecType& layersToCategorize, const categorizeType& catType, const CategorizeFilter& catFilter) const {
    LayerMap categorizedLayerMap;
    StrVecType foundCats;
    StrVecType relevantCats = layers.categoriesByType(catType);
    //loop over the requested categories, make sure they are found in the LayerCollection object
    for (auto iterCat = catFilter.categories.begin(); iterCat != catFilter.categories.end(); iterCat++) {
        bool found = std::find(relevantCats.begin(), relevantCats.end(), *iterCat) != relevantCats.end();
        if (found) {
            foundCats.push_back(*iterCat);
        }
    }
    if (catFilter.filterMode == CategorizeFilter::ONLY) { // constrain categories for ONLY
        relevantCats = foundCats;
    }

    for (auto iterLayer = layersToCategorize.begin(); iterLayer != layersToCategorize.end(); iterLayer++) {
        string layerName = utilities::getLayerFromChannel(*iterLayer);
        string dePrefixedLayerName = dePrefix(layerName);
        bool validLayer = false;
        for (auto iterFilter = catFilter.categories.begin(); iterFilter != catFilter.categories.end(); iterFilter++) {
            if (layers.isMember(*iterFilter, dePrefixedLayerName)) {
                validLayer = true;
            }
        }

        if ((validLayer & (catFilter.filterMode != CategorizeFilter::EXCLUDE)) ||
                (!validLayer & (catFilter.filterMode != CategorizeFilter::INCLUDE))) {

            for (auto iterCat = relevantCats.begin(); iterCat != relevantCats.end(); iterCat++) {

                if (layers.isMember(*iterCat, dePrefixedLayerName)) {
                    if (catFilter.filterMode != CategorizeFilter::ONLY) {
                        categorizedLayerMap.add("all", layerName);
                    }
                    categorizedLayerMap.add(*iterCat, layerName);
                }
            }
        }
    }
    return categorizedLayerMap;
};

LayerMap LayerCollection::topology(const StrVecType& layerNames, const topologyStyle& style) const {
    // in the channel config file, naming is defined by _vec4_exr _vec4
    const string defaultCategory =  "_vec4";
    const string exrToken = "_exr";
    const string defaultTopology = style == topologyStyle::exr ? defaultCategory + exrToken : defaultCategory;
    const StrVecType defaultChannels = channels[defaultTopology];

    const StrVecType topoNames = channels[TOPOLOGY_KEY_LEXICAL]; // all styles derived from lexical

    LayerMap channelMapping;
    LayerMap categorized = categorizeLayers(layerNames, categorizeType::priv);

    for (auto iterCategory = topoNames.begin(); iterCategory != topoNames.end(); iterCategory++) {
        if (!categorized.contains(*iterCategory)) {
            string _topoType = style == topologyStyle::exr ? *iterCategory + exrToken : *iterCategory;
            StrVecType channelNames = channels[_topoType]; // look up channel names for this type
            StrVecType categorizedLayers = categorized[*iterCategory];
            for (auto iterLayer = categorizedLayers.begin(); iterLayer != categorizedLayers.end(); iterLayer++) {
                channelMapping.strMap[*iterLayer] =  utilities::applyChannelNames(*iterLayer, channelNames);
            }
        }
    }
    // this is for layers that are were not classified, make them RGBA
    for (auto iterLayer = layerNames.begin(); iterLayer != layerNames.end(); iterLayer++) {
        // in case channel names are used, get the layer name
        string layerName = utilities::getLayerFromChannel(*iterLayer);
        if (!channelMapping.contains(layerName)) {
            channelMapping.strMap[layerName] = utilities::applyChannelNames(layerName, defaultChannels);
        }
    }
    return channelMapping;
};
