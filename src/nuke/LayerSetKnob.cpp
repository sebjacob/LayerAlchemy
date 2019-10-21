#include "LayerSetKnob.h"

namespace LayerSet {

LayerSetKnobData::LayerSetKnobData() {
   //printf("created LayerSetKnobData %p\n", (void*) this);
}

LayerSetKnobData::~LayerSetKnobData() {
   //printf("destroyed LayerSetKnobData %p\n", (void*) this);
}

DD::Image::Knob* LayerSetKnob(DD::Image::Knob_Callback& f, LayerSetKnobData& pLayerSetKnobData) {
    DD::Image::Knob* layerSetKnob = DD::Image::Enumeration_knob(
        f, &pLayerSetKnobData.selectedLayerSetIndex, pLayerSetKnobData.items, "layer_set", "layer set");
    Tooltip(f, "This selects a specific layer set for processing in this node");
    SetFlags(f, DD::Image::Knob::SAVE_MENU);
    SetFlags(f, DD::Image::Knob::ALWAYS_SAVE);
    SetFlags(f, DD::Image::Knob::EXPAND_TO_CONTENTS);
    SetFlags(f, DD::Image::Knob::STARTLINE);
    return layerSetKnob;
}
void setLayerSetNodeLabel(DD::Image::Op* t_op) {
    const char* label = getLayerSetKnobEnumString(t_op).c_str();
    t_op->knob("label")->set_text(label);
}

string getLayerSetKnobEnumString(DD::Image::Op* t_op) {
    return t_op->knob(LAYER_SET_KNOB_NAME)->enumerationKnob()->getSelectedItemString();
}
void populateLayerSetKnobEnum(DD::Image::Op* t_op, std::vector<string>& items) {
    t_op->knob(LAYER_SET_KNOB_NAME)->enumerationKnob()->menu(items);
}
void _updateLayerSetKnobEnum(DD::Image::Op* t_op, LayerSetKnobData& layerSetKnobData, ChannelSetMapType& channelSetLayerMap, const DD::Image::ChannelSet& inChannels) {
    DD::Image::Knob* layerSetKnob = t_op->knob(LAYER_SET_KNOB_NAME);
    DD::Image::Enumeration_KnobI* layerSetEnumKnob = layerSetKnob->enumerationKnob();
    int selectedIndex = layerSetEnumKnob->getSelectedItemIndex();
    StrVecType newCategories = getCategories(channelSetLayerMap);
    StrVecType currentCategories = layerSetEnumKnob->menu();
    string layerSetName = layerSetEnumKnob->getSelectedItemString();
    
    int indexCurrentItem = std::find(newCategories.begin(), newCategories.end(), layerSetEnumKnob->getSelectedItemString()) - newCategories.begin();
    int categoryAmount = newCategories.size();
    if (categoryAmount == 0) {
        StrVecType blank = {""};
        layerSetEnumKnob->menu(blank);
        layerSetKnob->set_value(0);
        layerSetKnobData.m_selectedChannels = DD::Image::ChannelSet(DD::Image::Chan_Black);
        return;
    } else if (indexCurrentItem < categoryAmount) {
        selectedIndex = indexCurrentItem;
    } else if (selectedIndex >= categoryAmount) {
        selectedIndex = 0;
    }
    layerSetEnumKnob->menu(newCategories);
    layerSetKnob->set_value(selectedIndex);

    layerSetKnobData.m_selectedChannels.clear();
    layerSetKnobData.m_selectedChannels += channelSetLayerMap[layerSetName];
    layerSetKnobData.categoryName = layerSetName;
    layerSetKnobData.m_allChannels = inChannels;
    //printf("_updateLayerSetKnobEnum categorized %s\n", layerSetName.c_str());
}
void updateLayerSetKnob(DD::Image::Op* t_op, LayerSetKnobData& layerSetKnobData, LayerCollection& collection, DD::Image::ChannelSet& inChannels) {
    if (!inChannels.empty()) {
        ChannelSetMapType channelSetLayerMap = categorizeChannelSet(collection, inChannels);
        _updateLayerSetKnobEnum(t_op, layerSetKnobData, channelSetLayerMap, inChannels);
    } else {
        layerSetKnobData.m_selectedChannels = DD::Image::ChannelSet(DD::Image::Chan_Black);
    }
}
void updateLayerSetKnob(DD::Image::Op* t_op, LayerSetKnobData& layerSetKnobData, LayerCollection& collection, DD::Image::ChannelSet& inChannels, const CategorizeFilter& categorizeFilter) {
    if (!inChannels.empty()) {
        ChannelSetMapType channelSetLayerMap = categorizeChannelSet(collection, inChannels, categorizeFilter);
        _updateLayerSetKnobEnum(t_op, layerSetKnobData, channelSetLayerMap, inChannels);
    } else {
        layerSetKnobData.m_selectedChannels = DD::Image::ChannelSet(DD::Image::Chan_Black);
    }
}

bool _preValidateLayerSetKnobUpdate(DD::Image::Op* t_op, const LayerSetKnobData& layerSetKnobData, const DD::Image::ChannelSet& inChannels)
{
    string currentLayerName = getLayerSetKnobEnumString(t_op);
    //printf("validateLayerSetKnobUpdate categorizeFilter : %s\n", currentLayerName.c_str());
    bool requiresUpdate = false;

    if (inChannels == DD::Image::Chan_Black || inChannels.empty()) 
    {
        // printf("validateLayerSetKnobUpdate categorizeFilter : empty input channels\n");
        return false;
    }
    else if (layerSetKnobData.m_allChannels.empty() || layerSetKnobData.m_allChannels != inChannels)// || !inChannels.contains(layerSetKnobData.m_selectedChannels))
    {
        // printf("validateLayerSetKnobUpdate categorizeFilter : different input channelset or m_allChannels empty=%d\n", layerSetKnobData.m_allChannels.empty());
        requiresUpdate = true;
    }
    else if (currentLayerName != layerSetKnobData.categoryName)
    {
        // printf("validateLayerSetKnobUpdate categorizeFilter : category changed\n");
        requiresUpdate = true;
    }
    else if (currentLayerName == layerSetKnobData.categoryName)
    {
        // printf("validateLayerSetKnobUpdate categorizeFilter : category is the same\n");
        requiresUpdate = false;
    }
    return requiresUpdate;
}

bool validateLayerSetKnobUpdate(DD::Image::Op* t_op, const LayerSetKnobData& layerSetKnobData, LayerCollection& layerCollection, const DD::Image::ChannelSet& inChannels) {
    string currentLayerName = getLayerSetKnobEnumString(t_op);
    bool valid = _preValidateLayerSetKnobUpdate(t_op, layerSetKnobData, inChannels);
    if (!valid) {
        return false;
    }
    StrVecType inLayers = LayerSet::getLayerNames(inChannels);
    LayerMap categorized = layerCollection.categorizeLayers(inLayers, categorizeType::pub);
    //printf("validateLayerSetKnobUpdate categorizeFilter : used categorized Layers \n");

    bool validLayerSet = currentLayerName == "" ? true : categorized.contains(currentLayerName);

    if (validLayerSet) {
        //printf("validateLayerSetKnobUpdate categorizeFilter : categorizeLayers has %s true\n", currentLayerName.c_str());
        return true;
    } else {
        t_op->error("input is missing the '%s' layer set", currentLayerName.c_str());
        return false;
    }
}

bool validateLayerSetKnobUpdate(DD::Image::Op* t_op, const LayerSetKnobData& layerSetKnobData, LayerCollection& layerCollection, const DD::Image::ChannelSet& inChannels, const CategorizeFilter& categorizeFilter) {
    string currentLayerName = getLayerSetKnobEnumString(t_op);
    bool valid = _preValidateLayerSetKnobUpdate(t_op, layerSetKnobData, inChannels);
    if (!valid) {
        return false;
    }
    StrVecType inLayers = LayerSet::getLayerNames(inChannels);
    LayerMap categorized = layerCollection.categorizeLayers(inLayers, categorizeType::pub, categorizeFilter);
    // printf("validateLayerSetKnobUpdate categorizeFilter : used categorized Layers \n");

    bool validLayerSet = currentLayerName == "" ? true : categorized.contains(currentLayerName);

    if (validLayerSet) {
        // printf("validateLayerSetKnobUpdate categorizeFilter : categorizeLayers has %s true\n", currentLayerName.c_str());
        return true;
    } else {
        t_op->error("input is missing the '%s' layer set", currentLayerName.c_str());
        return false;
    }
}
}; //  LayerSet
