#pragma once
#include <DDImage/ChannelSet.h>
#include <DDImage/Knobs.h>
#include <DDImage/Knob.h>
#include <DDImage/Enumeration_KnobI.h>
#include <DDImage/Row.h>

#include "LayerSetCore.h"

namespace LayerAlchemy {

    //layerCollection initialized here once.
    static LayerCollection layerCollection;
    //using DD::Image::ChannelSet;
    typedef map<string, DD::Image::ChannelSet> ChannelSetMapType;

namespace LayerSet {
    typedef map<string, DD::Image::ChannelSet> ChannelSetMapType;
    //Gets a vector of unique layer names for a ChannelSet
    StrVecType getLayerNames(const DD::Image::ChannelSet&);
    StrVecType getCategories(const ChannelSetMapType&);
    DD::Image::ChannelSet getChannelSet(const ChannelSetMapType&);

    ChannelSetMapType categorizeChannelSet(const LayerCollection&, const DD::Image::ChannelSet&);
    ChannelSetMapType categorizeChannelSet(const LayerCollection&, const DD::Image::ChannelSet&, const CategorizeFilter&);
    ChannelSetMapType _layerMaptoChannelMap(const LayerMap&, const DD::Image::ChannelSet& inChannels);
} //  End namespace LayerSet

namespace Utilities {
    void hard_copy(const DD::Image::Row& fromRow, int x, int r, DD::Image::ChannelSet channels, DD::Image::Row& toRow);
    float* hard_copy(const DD::Image::Row& fromRow, int x, int r, DD::Image::Channel channel, DD::Image::Row& toRow);
    // use to validate if a target layer the user selects is within the required color ranges
    void validateTargetLayerColorIndex(DD::Image::Op* t_op, const DD::Image::ChannelSet& targetLayer, unsigned minIndex, unsigned maxIndex);
} //  End namespace Utilities

namespace Knobs {
    DD::Image::Knob* createDocumentationButton(DD::Image::Knob_Callback&);
    DD::Image::Knob* createColorKnobResetButton(DD::Image::Knob_Callback&);
} //  End namespace Knobs
} //  End namespace LayerAlchemy
