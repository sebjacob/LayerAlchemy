#pragma once
#include <DDImage/ChannelSet.h>
#include <DDImage/Knobs.h>
#include <DDImage/Knob.h>
#include <DDImage/Enumeration_KnobI.h>

#include "LayerSetCore.h"

//layerCollection initialized here once.
static LayerCollection layerCollection;

namespace LayerSet
{
    //alias type to for string:ChannelSet map
    typedef map<string, DD::Image::ChannelSet> ChannelSetMapType;
    //Gets a vector of unique layer names for a ChannelSet
    StrVecType getLayerNames(const DD::Image::ChannelSet&);
    ChannelSetMapType categorizeChannelSet(const LayerCollection&, const DD::Image::ChannelSet&);
    ChannelSetMapType categorizeChannelSet(const LayerCollection&, const DD::Image::ChannelSet&, const CategorizeFilter&);
    ChannelSetMapType _layerMaptoChannelMap(const LayerMap&, const DD::Image::ChannelSet& inChannels);
    StrVecType getCategories(const ChannelSetMapType&);
    DD::Image::ChannelSet getChannelSet(const ChannelSetMapType&);
    DD::Image::Knob* createDocumentationButton(DD::Image::Knob_Callback&);
    DD::Image::Knob* createColorKnobResetButton(DD::Image::Knob_Callback&);
    void validateTargetLayerColorIndex(DD::Image::Op* t_op, const DD::Image::ChannelSet& targetLayer, unsigned minIndex, unsigned maxIndex);
}; //  LayerSet
