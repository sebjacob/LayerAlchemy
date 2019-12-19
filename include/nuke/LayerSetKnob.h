#pragma once
#include <DDImage/ChannelSet.h>
#include <DDImage/Knobs.h>
#include <DDImage/Knob.h>
#include <DDImage/Enumeration_KnobI.h>

#include "LayerSet.h"

namespace LayerAlchemy {
namespace LayerSetKnob {

static const char* LAYER_SET_KNOB_NAME = "layer_set";

/**
 * Stores the data needed to manage LayerSet enumeration knob
 */
class LayerSetKnobData {
public:
    // stores the last used LayerSet name, useful to compare if an update is required, or can be skipped
    string categoryName {""};
    // stores the list of categories in the enumeration knob
    const char* items[2] = {"", 0};
    //stores the index of the the enumeration knob
    int selectedLayerSetIndex {0};
    // stores the result ChannelSet filtered by the knob.
    DD::Image::ChannelSet m_selectedChannels;
    // stores the ChannelSet last used, useful to compare if an update is required, or can be skipped
    DD::Image::ChannelSet m_allChannels;
    LayerSetKnobData();
    ~LayerSetKnobData();
};

/**
 * convenience functions directly related to creating and manipulating the enumeration knob
 */

// centralized function for creating LayerSet Enumeration Knob in a node's knobs() function
DD::Image::Knob* LayerSetKnob(DD::Image::Knob_Callback&, LayerSetKnobData&);
// get the current LayerSet on an Op
string getLayerSetKnobEnumString(DD::Image::Op*);
void populateLayerSetKnobEnum(DD::Image::Op*, std::vector<string>&);

/**
 * validation functions that manage if going through an reconfiguration of the enumeration knob is necessary
 * Reasons :
 * - Nuke runs _validate a lot, this eliminates wasting cpu cycles, when possible
 * - Reliability, we want to give the user a chance to fix any errors, so this handles the assertions
 */

// first stage of update handling : inexpensive check to see if an update is required
bool _basicValidateLayerSetKnobUpdate(DD::Image::Op*, const LayerSetKnobData&, const DD::Image::ChannelSet&);
// second stage of update handling : categorizes the incoming channels, and decides if an update is redundant.
bool _categorizedValidateLayerSetKnobUpdate(DD::Image::Op*, const LayerMap&, const string&);
// main validation function for managing LayerSetKnob updating
bool validateLayerSetKnobUpdate(DD::Image::Op*, const LayerSetKnobData&, LayerCollection&, const DD::Image::ChannelSet&);
// main validation filtered function for managing LayerSetKnob updating
bool validateLayerSetKnobUpdate(DD::Image::Op*, const LayerSetKnobData&, LayerCollection&, const DD::Image::ChannelSet&, const CategorizeFilter&);

/**
 * convenience functions for doing the actual updating of the updating of both the knob and it's storage
 */

// private function to do the actual data updating to the enumeration knob
void _updateLayerSetKnob(DD::Image::Op*, LayerSetKnobData&, ChannelSetMapType&, const DD::Image::ChannelSet&);
// main update function to update an Op's LayerSetKnob
void updateLayerSetKnob(DD::Image::Op*, LayerSetKnobData&, LayerCollection&, DD::Image::ChannelSet&);
// main update function to update an Op's filtered LayerSetKnob
void updateLayerSetKnob(DD::Image::Op*, LayerSetKnobData&, LayerCollection&, DD::Image::ChannelSet&, const CategorizeFilter&);
} //  End namespace LayerSetKnob
} //  End namespace LayerAlchemy
