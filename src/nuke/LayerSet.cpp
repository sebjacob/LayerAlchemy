#include "LayerSet.h"

namespace LayerAlchemy {
namespace LayerSet {

StrVecType getLayerNames(const DD::Image::ChannelSet& inChannels)
{
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

ChannelSetMapType _layerMaptoChannelMap(const LayerMap& layerMap, const DD::Image::ChannelSet& inChannels)
{
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

ChannelSetMapType categorizeChannelSet(const LayerCollection& collection, const DD::Image::ChannelSet& inChannels)
{
    StrVecType inLayers = LayerSet::getLayerNames(inChannels);
    LayerMap layerMap = collection.categorizeLayers(inLayers, categorizeType::pub);
    return _layerMaptoChannelMap(layerMap, inChannels);
}

ChannelSetMapType categorizeChannelSet(const LayerCollection& collection, const DD::Image::ChannelSet& inChannels, const CategorizeFilter& categorizeFilter)
{
    StrVecType inLayers = LayerSet::getLayerNames(inChannels);
    LayerMap layerMap = collection.categorizeLayers(inLayers, categorizeType::pub, categorizeFilter);
    layerMap.strMap.erase("all"); // not useful for Nuke when CategorizeFilter is used
    return _layerMaptoChannelMap(layerMap, inChannels);
}

StrVecType getCategories(const ChannelSetMapType& channelSetLayerMap)
{
    StrVecType categories;
    categories.reserve(channelSetLayerMap.size());
    for (auto& kvp : channelSetLayerMap) {
        categories.emplace_back(kvp.first);
    }
    return categories;
}

DD::Image::ChannelSet getChannelSet(const ChannelSetMapType& channelSetLayerMap)
{
    DD::Image::ChannelSet channels;
    for (auto& kvp : channelSetLayerMap) {
        channels += kvp.second;
    }
    return channels;
}

} // End namespace LayerSet
namespace Knobs {

DD::Image::Knob* createDocumentationButton(DD::Image::Knob_Callback& f)
{
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
DD::Image::Knob* createVersionTextKnob(DD::Image::Knob_Callback& f)
{
    string label = "<font color='DimGrey'>v" + LAYER_ALCHEMY_VERSION_STRING + "</font>";
    DD::Image::Knob* versionTextKnob = Text_knob(f, label.c_str());
    return versionTextKnob;
}


} // End namespace Knobs

namespace Utilities {

void validateTargetLayerColorIndex(DD::Image::Op* t_op, const DD::Image::ChannelSet& targetLayer, unsigned minIndex, unsigned
maxIndex)
{
    foreach(channel, targetLayer)
    {
        unsigned colorIdx = colourIndex(channel);
        if (colorIdx < minIndex || colorIdx > maxIndex) {
            t_op->error("target layer's current channels are out of color range");
        }
    }
}

float* hard_copy(const DD::Image::Row& fromRow, int x, int r, DD::Image::Channel channel, DD::Image::Row& toRow)
{
    float* outAovValue;
    if (fromRow.is_zero(channel)) 
    {
        float* outAovValue = toRow.writableConstant(0.0f, channel);
    } 
    else
    {
        const float* inAovValue = fromRow[channel];
        float* outAovValue = toRow.writable(channel);
        for (int X = x; X < r; X++) 
        {
            outAovValue[X] = inAovValue[X];
        }
    }
    return outAovValue;
}

void hard_copy(const DD::Image::Row& fromRow, int x, int r, DD::Image::ChannelSet channels, DD::Image::Row& toRow)
{
    foreach(channel, channels)
    {
      hard_copy(fromRow, x, r, channel, toRow);
    }
}
} // End namespace Utilities
} // End namespace LayerAlchemy

