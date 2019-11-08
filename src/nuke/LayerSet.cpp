#include "LayerSet.h"

namespace LayerSet {

StrVecType getLayerNames(const DD::Image::ChannelSet& inChannels) {
    DD::Image::ChannelSet selectedChannels;
    StrVecType layerNames;

    foreach(z, inChannels) {
        string layer = DD::Image::getLayerName(z);
        bool contains = (find(begin(layerNames), end(layerNames), layer)) != layerNames.end();

        if (!contains) {
            layerNames.emplace_back(layer);
        }
    }
    return layerNames;
}

DD::Image::Knob* createDocumentationButton(DD::Image::Knob_Callback& f) {
    DD::Image::Knob* docButton = Button(f, "docButton", "documentation");
    Tooltip(f, "<p>This will launch the default browser and load the included plugin documentation</p>");
    return docButton;
}

DD::Image::Knob* createColorKnobResetButton(DD::Image::Knob_Callback& f) {
    const char* colorKnobResetScript =
    "currentNode = nuke.thisNode()\n"
    "colorKnobs = [knob for knob in currentNode.allKnobs() if isinstance(knob, nuke.Color_Knob)]\n"
    "for knob in colorKnobs:\n"
    "    knob.clearAnimated()\n"
    "    knob.setValue(knob.defaultValue())\n";
    return PyScript_knob(f, colorKnobResetScript, "reset values");
}

ChannelSetMapType _layerMaptoChannelMap(const LayerMap& layerMap, const DD::Image::ChannelSet& inChannels) {
    ChannelSetMapType channelSetLayerMap;
    for (auto& kvp : layerMap.strMap) {
        foreach (channel, inChannels) {
            string layerName = getLayerName(channel);
            if (layerMap.isMember(kvp.first, layerName)) {
                channelSetLayerMap[kvp.first].insert(channel);
            }
        }
    }
    return channelSetLayerMap;
}

ChannelSetMapType categorizeChannelSet(const LayerCollection& collection, const DD::Image::ChannelSet& inChannels) {
    StrVecType inLayers = LayerSet::getLayerNames(inChannels);
    LayerMap layerMap = collection.categorizeLayers(inLayers, categorizeType::pub);
    return _layerMaptoChannelMap(layerMap, inChannels);
}

ChannelSetMapType categorizeChannelSet(const LayerCollection& collection, const DD::Image::ChannelSet& inChannels, const CategorizeFilter& categorizeFilter) {
    StrVecType inLayers = LayerSet::getLayerNames(inChannels);
    LayerMap layerMap = collection.categorizeLayers(inLayers, categorizeType::pub, categorizeFilter);
    return _layerMaptoChannelMap(layerMap, inChannels);
}

StrVecType getCategories(const ChannelSetMapType& channelSetLayerMap) {
    StrVecType categories;
    categories.reserve(channelSetLayerMap.size());
    for (auto& kvp : channelSetLayerMap) {
        categories.emplace_back(kvp.first);
    }
    return categories;
}

DD::Image::ChannelSet getChannelSet(const ChannelSetMapType& channelSetLayerMap) {
    DD::Image::ChannelSet channels;
    for (auto& kvp : channelSetLayerMap) {
        channels += kvp.second;
    }
    return channels;
}
}; // LayerSet

