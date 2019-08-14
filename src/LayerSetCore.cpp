/* 
 * File:   LayerSetCore.cpp
 * Author: sjacob
 * 
 * Created on October 9, 2018, 10:52 p.m.
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

LayerMap::LayerMap(const string yamlFilePath) : strMap(loadConfigToMap(yamlFilePath)) {
}

StrVecType LayerMap::operator[](const StrVecType& categoryNames) {
    StrVecType items;
    StrVecType::const_iterator it, it2;

    for (it = categoryNames.begin(); it != categoryNames.end(); it++) {
        StrVecType catego = strMap[*it];

        if (catego.size() != 0) {
            for (it2 = catego.begin(); it2 != catego.end(); it2++) {
                items.push_back(*it2);
            }
        }
    }
    return items;
}

StrVecType LayerMap::operator[](const string category) {
    StrVecType items;
    if (strMap.find(category) != strMap.end()) {
        return strMap[category];
    } else {
        return items;
    }
}

bool LayerMap::isMember(const string& categoryName, const string& layer) const {
    StrVecType layerVec = strMap.find(categoryName)->second;
    bool contains = find(begin(layerVec), end(layerVec), layer) != layerVec.end();
    return contains;
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
        const string& categoryName = kvp.first;
        if (0 != categoryName.find("_")) {
            itemKeys.emplace_back(categoryName);
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
    if (unPrefixedLayerName.length() > 0) {
        outputLayerName = unPrefixedLayerName;
    } else {
        outputLayerName = layerName;
    }

    return outputLayerName;
}

bool LayerMap::contains(const string& element) const {
    StrVecType categories = this->categories();
    bool result = std::any_of(categories.begin(), categories.end(), [element](const string & str) {
        return str == element; });
    return result;
}

bool LayerMap::contains(const StrVecType& elements) const {
    StrVecType categories = this->categories();
    return std::includes(categories.begin(), categories.end(), elements.begin(), elements.end());
}

string utilities::getLayerFromChannel(const string& layer) {
    string dot = ".";
    auto end = layer.find(dot);
    return layer.substr(0, end);
};

StrVecType utilities::applyChannelNames(const string& layerName, const StrVecType& topologyVector) {
    StrVecType channelNames;
    channelNames.reserve(topologyVector.size());
    for (StrVecType::const_iterator iterChannel = topologyVector.begin(); iterChannel != topologyVector.end(); iterChannel++) {
        string chan = layerName + "." + *iterChannel;
        channelNames.emplace_back(chan);
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

LayerMap LayerCollection::topology(const StrVecType& layerNames, const topologyStyle& reference) {
    StrVecType::const_iterator iterCategory, iterLayer, iterChannel;
    string topoKey = "_vec4";
    if (reference == topologyStyle::exr) {
        topoKey += "_exr";
    }

    const StrVecType& topoNames = channels["topology"];
    const StrVecType& vecTopoRGBA = channels[topoKey];

    LayerMap channelMapping;
    LayerMap categorized = categorizeLayers(layerNames, categorizeType::priv);

    for (iterCategory = topoNames.begin(); iterCategory != topoNames.end(); iterCategory++) {
        // todo, implement "hasAny" and "hasAll" methods to LayerMap class
        if (!categorized.contains(*iterCategory)) {
            const StrVecType& categorizedLayers = categorized[*iterCategory];
            string _topoType = *iterCategory;
            if (reference == topologyStyle::exr) {
                _topoType = *iterCategory + "_exr";
            }
            const StrVecType& thisTopo = channels[_topoType];
            for (iterLayer = categorizedLayers.begin(); iterLayer != categorizedLayers.end(); iterLayer++) {
                StrVecType channelNames = utilities::applyChannelNames(*iterLayer, thisTopo);
                channelMapping.strMap[*iterLayer] = channelNames;
            }
        }
    }
    for (iterLayer = layerNames.begin(); iterLayer != layerNames.end(); iterLayer++) {
        string layerName = utilities::getLayerFromChannel(*iterLayer); // in case
        if (!channelMapping.contains(layerName)) {
            StrVecType channelNames = utilities::applyChannelNames(layerName, vecTopoRGBA);
            channelMapping.strMap[layerName] = channelNames;
        }
    }
    return channelMapping;
};
