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
    const char* docButtonScript = 
    "import layer_alchemy.documentation\n"
    "qtWidget = layer_alchemy.documentation.displayDocumentation(node=nuke.thisNode())\n"
    "if qtWidget:\n"
    "    qtWidget.show()";

    DD::Image::Knob* docButton = PyScript_knob(f, docButtonScript, "documentation");
    Tooltip(f, "<p>This will display the included plugin documentation</p>");
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

void gradeChannelPixelEngine(const DD::Image::Row& in, int y, int x, int r, DD::Image::ChannelSet& channels, DD::Image::Row& aRow, float* A, float* B, float* G, bool reverse, bool clampBlack, bool clampWhite)
{
    // patch for linux alphas because the pow function behaves badly
    // for very large or very small exponent values.
    static bool LINUX = false;
    #ifdef __alpha
    LINUX = true;
    #endif


    map<DD::Image::Channel, float*> aovPtrIdxMap;
    foreach(channel, channels)
    {
        LayerAlchemy::Utilities::hard_copy(in, x, r, channel, aRow);
        aovPtrIdxMap[channel] = aRow.writable(channel);
    }

    foreach(channel, channels) {
        unsigned chanIdx = colourIndex(channel);
        const float* inAovValue = in[channel];
        float* outAovValue = aovPtrIdxMap[channel];

        float _A = A[chanIdx];
        float _B = B[chanIdx];
        float _G = G[chanIdx];

        for (int X = x; X < r; X++)
        {
            float outPixel = inAovValue[X];

            if (!reverse) {
                if (_A != 1.0f || _B) {
                    outPixel *= _A;
                    outPixel += _B;
                    }
                if (clampWhite || clampBlack) {
                    if (outPixel < 0.0f && clampBlack) { // clamp black
                        outPixel = 0.0f;
                    }
                    if (outPixel > 1.0f && clampWhite) { // clamp white
                        outPixel = 1.0f;
                    }
                }
                if (_G <= 0) {
                    if (outPixel < 1.0f) {
                        outPixel = 0.0f;
                    } else if (outPixel > 1.0f) {
                        outPixel = INFINITY;
                    }
                } else if (_G != 1.0f) {
                    float power = 1.0f / _G;
                    if (LINUX & (outPixel <= 1e-6f && power > 1.0f)) {
                        outPixel = 0.0f;
                    } else if (outPixel < 1) {
                        outPixel = powf(outPixel, power);
                    } else {
                        outPixel = (1.0f + outPixel - 1.0f) * power;
                    }
                }
            }
            if (reverse) { // Reverse gamma:
                if (_G <= 0) {
                    outPixel = outPixel > 0.0f ? 1.0f : 0.0f;
                }
                if (_G != 1.0f) {
                    if (LINUX & (outPixel <= 1e-6f && _G > 1.0f)) {
                        outPixel = 0.0f;
                    } else if (outPixel < 1.0f) {
                        outPixel = powf(outPixel, _G);
                    } else {
                        outPixel = 1.0f + (outPixel - 1.0f) * _G;
                    }
                }
                // Reverse the linear part:
                if (_A != 1.0f || _B) {
                    float b = _B;
                    float a = _A;
                    if (a) {
                        a = 1 / a;
                    } else {
                        a = 1.0f;
                    }
                    b = -b * a;
                    outPixel = (outPixel * a) + b;
                }
            }
            // clamp
            if (clampWhite || clampBlack) {
                if (outPixel < 0.0f && clampBlack)
                {
                    outPixel = 0.0f;
                }
                else if (outPixel > 1.0f && clampWhite)
                {
                    outPixel = 1.0f;
                }
            }
            outAovValue[X] = outPixel;
        }
    }
}

float validateGammaValue(const float& gammaValue) 
{
    static bool LINUX = false;
    #ifdef __alpha
    LINUX = true;
    #endif
    if (LINUX) 
    {
        if (gammaValue < 0.008f) 
        {
            return 0.0f;
        } 
        else if (gammaValue > 125.0f) 
        {
            return 125.0f;
        }
    }
    return gammaValue;
}
} // End namespace Utilities
} // End namespace LayerAlchemy

