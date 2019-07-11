#pragma once
#include <ctime>

#include <DDImage/ChannelSet.h>
#include <DDImage/Knobs.h>
#include <DDImage/Knob.h>
#include <DDImage/Enumeration_KnobI.h>

#include "LayerSetCore.h"

//layerCollection initialized here once.
static LayerCollection layerCollection;

namespace LayerSet {
    //alias type to for string:ChannelSet map
    typedef map<const string, DD::Image::ChannelSet> ChannelSetMapType;
    //Gets a vector of unique layer names for a ChannelSet
    StrVecType getLayerNames(const DD::Image::ChannelSet&);

    /**
     * Class to manage the data related to a LayerSetKnobWrapper instance 
     */
    class LayerSetKnobValueStore {
    public:
        //stores the ChannelSet last used, useful to compare if an update is required, or can be skipped
        DD::Image::ChannelSet channelSet { DD::Image::ChannelSet(DD::Image::Chan_Black)};
        //stores the layer set last used, useful to compare if an update is required, or can be skipped
        string categoryName {""};
        // stores full public layer categorization, regardless of the enumeration knob's update method
        LayerMap layerMap;
        //stores the list of categories in the enumeration knob
        const char* staticChoices[2] = {"", 0};
        //stores the index of the the enumeration knob
        int selectedLayerSetIndex {0};
        //knob pointer to the enumeration knob
        DD::Image::Knob* selectedLayerKnob;
        //stores the result ChannelSet filtered by the knob. Basically, Channels that are in the selected LayerSet
        DD::Image::ChannelSet selectedChannels; 
    };
    /**
     * Wrapper class for adding a "LayerSet enabled Enumeration Knob"
     * 
     * This is a dynamic enumeration knob that is connected to the layer characterization engine.
     * Avoids code duplication, since it is part of every tool in this project.
     * Provides an interface to store, update, and validate multichannel streams
     * It also provides a way to filter incoming channels in groups, and a tool can use this to select or define the 
     * input/output of channels to operate on.
     * 
     * Following example the call is called "NukeNodeClass"
     * 
     * Used in this project as a public object in a class:
     * 
     * class NukeNodeClass : public Iop {
     * public:
     *      LayerSetKnobWrapper layerSetKnob;
     * 
     * Then, since it is a knob after all, add it, like any other knob in the knobs function :
     * 
     * void NukeNodeClass::knobs(Knob_Callback f) {
     *      layerSetKnob.setCallbacks(f);
     * 
     * To update the contents, a natural place to do so is in _validate
     * includes public functions to control if an update should be done, as _validate is run quite a lot, so it
     * is more efficient to only do so when actually necessary and safe to do so.
     * 
     * Will throw an Op error and halt if the input channels are implausible, prevents a cascade of unfortunate
     * knob reconfigurations in a script without user consent (user has no control over _validate).
     */
    class LayerSetKnobWrapper {
    private:
        //sets menu items on the knob
        void setMenu(const StrVecType&);
        //sets the knob to a specific index value
        void setItem(const int);
        void setEnumKnob(DD::Image::Knob*);
        //current layer set  name set on the enumeration knob
        int getSelectedItemIndex();
        LayerSetKnobValueStore* ptrLayerSetKnobValueStore;

    public:
        //default constructor, must be passed with a LayerSetKnobValueStore object
        LayerSetKnobWrapper(LayerSetKnobValueStore&);
        LayerSetKnobWrapper();
        //default destructor
        ~LayerSetKnobWrapper();
        //pointer to current ChannelSet configured in this instance
        const DD::Image::ChannelSet* ptrConfiguredChannels;
        //returns a new ChannelSet  with channels configured in this instance
        DD::Image::ChannelSet configuredChannelSet();
        //current layer set  name set on the enumeration knob, not the current configured item (user could have changed it)
        string getSelectedItemString();
        //current layer set  name configured in this instance, not the current layer set  name set on the enumeration knob
        string configuredLayerSetName();
        //returns <b>selected</b> the layer names configured in this instance.
        StrVecType configuredLayerNames();
        //returns all the layer set names configured in this instance
        const StrVecType configuredCategoryNames();
        //tests if the the current state contains a certain layer set, by name
        bool contains(const string&);
        //tests if a ChannelSet is equal to the current state, useful to quickly judge if any updating is required
        bool channelsChanged(const DD::Image::ChannelSet&);
        //tests if the enumeration knob's current value is the same value as the configured state
        bool categoryChanged();
        //this should be run before updating, will test the input ChannelSet and the current state of this instance
        bool validateChannels(DD::Image::Op*, LayerCollection&, const DD::Image::ChannelSet&);
        //private function to actually do the value setting and management of the knob
        void _update(const DD::Image::ChannelSet&, const StrVecType&);
        //update the object for a given ChannelSet, \n\n 
        //<i>(this normally goes in _validate)
        void update(LayerCollection&, DD::Image::ChannelSet&);
        //<b>selectively</b> update the object for a given ChannelSet\n\n
        //it uses the <i>CategorizeFilter</i> object to configure what categories to either:\n\n
        //to have only specific categories be considered : \n\n use <i>CategorizeFilter::ONLY</i> \n\n
        //to exclude categories use <i>CategorizeFilter::EXCLUDE</i>\n\n
        //<i>(this normally goes in _validate)
        void update(LayerCollection&, const DD::Image::ChannelSet&, const CategorizeFilter&);
        //convenience function for centralizing all the callback code for this object. Call this in the knobs() function to activate.
        DD::Image::Knob* createEnumKnob(DD::Image::Knob_Callback&);
        void setNodeLabel(DD::Image::Op*);
    };
    // add to constructor to check if the year (ex: 119 for 2019) and month (ex: 3 for march) are not expired
    void validateBeta(DD::Image::Op*, const int&, const int&);
    void createDocumentationButton(DD::Image::Knob_Callback&);
    void createColorKnobResetButton(DD::Image::Knob_Callback&);
}; //  LayerSet
