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
    
    LayerSetKnobWrapper::~LayerSetKnobWrapper() {
    }
    LayerSetKnobWrapper::LayerSetKnobWrapper() {
       ptrLayerSetKnobValueStore = new LayerSetKnobValueStore();
       ptrConfiguredChannels = &ptrLayerSetKnobValueStore->selectedChannels;
    }

    LayerSetKnobWrapper::LayerSetKnobWrapper(LayerSetKnobValueStore& ptr) {
        ptrLayerSetKnobValueStore = &ptr;
        ptrConfiguredChannels = &ptrLayerSetKnobValueStore->selectedChannels;
    }
    void LayerSetKnobWrapper::setEnumKnob(DD::Image::Knob* knob) {
        ptrLayerSetKnobValueStore->selectedLayerKnob = knob;
    }
    void LayerSetKnobWrapper::setMenu(const StrVecType& items) {
        ptrLayerSetKnobValueStore->selectedLayerKnob->enumerationKnob()->menu(items);
    }

    void LayerSetKnobWrapper::setItem(const int index) {
        ptrLayerSetKnobValueStore->selectedLayerKnob->set_value(index);
    }
    string LayerSetKnobWrapper::getSelectedItemString() {
        return ptrLayerSetKnobValueStore->selectedLayerKnob->enumerationKnob()->getSelectedItemString();
    }
    int LayerSetKnobWrapper::getSelectedItemIndex() {
        return ptrLayerSetKnobValueStore->selectedLayerKnob->enumerationKnob()->getSelectedItemIndex();
    }
    DD::Image::ChannelSet LayerSetKnobWrapper::configuredChannelSet() {
        return DD::Image::ChannelSet(ptrLayerSetKnobValueStore->selectedChannels);
    }
    string LayerSetKnobWrapper::configuredLayerSetName() {
        return ptrLayerSetKnobValueStore->categoryName;
    }

    StrVecType LayerSetKnobWrapper::configuredLayerNames() {
        return getLayerNames(ptrLayerSetKnobValueStore->selectedChannels);
    }

    const StrVecType LayerSetKnobWrapper::configuredCategoryNames() {
        return ptrLayerSetKnobValueStore->layerMap.categories();
    }

    bool LayerSetKnobWrapper::categoryChanged() {
        return (getSelectedItemString() != ptrLayerSetKnobValueStore->categoryName);
    }

    bool LayerSetKnobWrapper::channelsChanged(const DD::Image::ChannelSet& inChannels) {
        return (inChannels != ptrLayerSetKnobValueStore->channelSet);
    }

    bool LayerSetKnobWrapper::contains(const string& category) {
        return ptrLayerSetKnobValueStore->layerMap.contains(category);
    }

    bool LayerSetKnobWrapper::validateChannels(DD::Image::Op* opPtr, LayerCollection& layerCollection, const DD::Image::ChannelSet& inChannels) {
        if (inChannels == DD::Image::Chan_Black || inChannels.empty()) {
            return false;
        }
        if (ptrLayerSetKnobValueStore->categoryName == "") { // Nothing configured, worth updating
            return true;
        }
        StrVecType inLayers = LayerSet::getLayerNames(inChannels);
        bool validLayerSet = layerCollection.categorizeLayers(inLayers, categorizeType::pub).contains(ptrLayerSetKnobValueStore->categoryName);

        if (!validLayerSet) {
            opPtr->error("input is missing the '%s' layer set", ptrLayerSetKnobValueStore->categoryName.c_str());
            return false;
        }
        return true;
    }

    void LayerSetKnobWrapper::_update(const DD::Image::ChannelSet& inChannels, const StrVecType& categories) {
        int selectedIndex = getSelectedItemIndex();
        int indexCurrentItem = std::find(categories.begin(), categories.end(), getSelectedItemString()) - categories.begin();
        int categoryAmount = categories.size();
        if (categoryAmount == 0) {
            StrVecType blank = {""};
            setMenu(blank);
            setItem(0);
            ptrLayerSetKnobValueStore->selectedChannels = DD::Image::ChannelSet(DD::Image::Chan_Black);
            return;
        } else if (indexCurrentItem < categoryAmount) {
            selectedIndex = indexCurrentItem;
        } else if (selectedIndex >= categoryAmount) {
            selectedIndex = 0;
        }
        setMenu(categories);
        setItem(selectedIndex);

        ptrLayerSetKnobValueStore->categoryName = categories[selectedIndex];

        ptrLayerSetKnobValueStore->selectedChannels.clear();

        foreach(z, inChannels) {
            if (ptrLayerSetKnobValueStore->layerMap.isMember(ptrLayerSetKnobValueStore->categoryName, getLayerName(z))) {
                ptrLayerSetKnobValueStore->selectedChannels.insert(z);
            }
        }
        ptrLayerSetKnobValueStore->channelSet= inChannels;
    }

    DD::Image::Knob* LayerSetKnobWrapper::createEnumKnob(DD::Image::Knob_Callback& f) {
        using DD::Image::Knob;
        Knob *knob = DD::Image::Enumeration_knob(f, &ptrLayerSetKnobValueStore->selectedLayerSetIndex, ptrLayerSetKnobValueStore->staticChoices, "layer_set", "layer set");
        Tooltip(f, "This selects a specific layer set for processing in this node");
        SetFlags(f, Knob::SAVE_MENU);
        SetFlags(f, Knob::ALWAYS_SAVE);
        SetFlags(f, Knob::EXPAND_TO_CONTENTS);
        SetFlags(f, Knob::STARTLINE);
        ptrLayerSetKnobValueStore->selectedLayerKnob = knob;
        return knob;
    }

    void LayerSetKnobWrapper::update(LayerCollection& collection, DD::Image::ChannelSet& inChannels) {
        if (!inChannels.empty()) {
            StrVecType inLayers = LayerSet::getLayerNames(inChannels);
            ptrLayerSetKnobValueStore->layerMap = collection.categorizeLayers(inLayers, categorizeType::pub);
            _update(inChannels, configuredCategoryNames());

        } else {
            ptrLayerSetKnobValueStore->selectedChannels = DD::Image::ChannelSet(DD::Image::Chan_Black);
        }
    }

    void LayerSetKnobWrapper::setNodeLabel(DD::Image::Op* op) {
        op->knob("label")->set_text(getSelectedItemString().c_str());
    }

    void LayerSetKnobWrapper::update(LayerCollection& collection, const DD::Image::ChannelSet& inChannels, const CategorizeFilter& categorizeFilter) {

        if (!inChannels.empty()) {
            StrVecType inLayers = LayerSet::getLayerNames(inChannels);
            LayerMap filteredMap = collection.categorizeLayers(inLayers, categorizeType::pub, categorizeFilter);
            ptrLayerSetKnobValueStore->layerMap = collection.categorizeLayers(inLayers, categorizeType::pub);
            _update(inChannels, filteredMap.categories());
        } else {
            ptrLayerSetKnobValueStore->selectedChannels = DD::Image::ChannelSet(DD::Image::Chan_Black);
        }
    }
    
    void validateBeta(DD::Image::Op* nukeOpPtr, const int& year, const int& month) {
        std::time_t date = std::time(NULL);
        std::tm* ts = std::localtime(&date);
        if ((ts->tm_year >= year - 1900) && (ts->tm_mon > month -1)) {
            nukeOpPtr->set_unlicensed();
        }
    }
    void createDocumentationButton(DD::Image::Knob_Callback& f) {
        Button(f, "docButton", "documentation");
        Tooltip(f, "<p>This will launch the default browser and load the included plugin documentation</p>");
    }
    void createColorKnobResetButton(DD::Image::Knob_Callback& f) {
        const char* colorKnobResetScript =
        "currentNode = nuke.thisNode()\n"
        "colorKnobs = [knob for knob in currentNode.allKnobs() if isinstance(knob, nuke.Color_Knob)]\n"
        "for knob in colorKnobs:\n"
        "    knob.clearAnimated()\n"
        "    knob.setValue(knob.defaultValue())\n";
        PyScript_knob(f, colorKnobResetScript, "reset values");
    }
}; // LayerSet

