#include <memory>
#include "math.h"

#include <DDImage/Row.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>

#include "LayerSet.h"

static const char* const HELP =
        "Provides artist friendly controls to manipulate multichannel cg render passes\n\n"
        "GradeBeauty will dynamically reconfigure itself depending on : \n\n"
        "    * layers found connected to its input\n"
        "    * categorization of found layer names based on what is defined in the configuration files\n\n"
        "Any color knob change affects :\n\n"
        "    * the related multichannel layers\n"
        "    * the beauty pass is then rebuilt with the modified layers\n\n"
        "Configurations: \n\n"
        "    * default configuration is Arnold 5 stock layer names\n"
        "    * can be adapted to any additive type renderer\n"
        "    * To change this, you must modify the configuration files accordingly\n\n"
        "    * <i>more info with the documentation button</i>"
        ;

static const char* DEFAULT_VALUE_PYSCRIPT =
    "node = nuke.thisNode()\n"
    "colorKnobs = [knob for knob in node.allKnobs() if isinstance(knob, nuke.Color_Knob)]\n"
    "default = node.knob('math_type').getValue()\n"
    "defaultValue = [default] * 3\n"
    "for knob in colorKnobs:\n"
    "    if knob.defaultValue() != default:\n"
    "        knobToScript = knob.toScript()\n"
    "        knob.setDefaultValue(defaultValue)\n"
    "        knob.fromScript(knobToScript)"
;
static const char* const CLASS = "GradeBeauty";

namespace BeautyLayerSetConstants {
    // frequently used lists of layer set category names 
    namespace categories {
        static const StrVecType all = {
            "beauty_direct_indirect", "beauty_shading_global", "light_group", "beauty_shading"
        };
        static const string shading = "beauty_shading";
        static const StrVecType nonShading = {"beauty_direct_indirect", "beauty_shading_global", "light_group"};
        static const StrVecType global = {"beauty_shading_global", "beauty_direct_indirect"};
    }
    // frequently used lists of layers 
    namespace layers {
        static const StrVecType all = layerCollection.layers[categories::all];
        static const StrVecType shading = layerCollection.layers[categories::shading];
        static const StrVecType nonShading = layerCollection.layers[categories::nonShading];
        static const StrVecType global = layerCollection.layers[categories::global];
    };
}

using namespace DD::Image;
using namespace LayerSet;
using namespace BeautyLayerSetConstants;

static const CategorizeFilter CategorizeFilterAllBeauty(categories::all, CategorizeFilter::modes::ONLY);

//The two modes that define the default value of this node's color knobs, and the math formula to apply
enum MATH_MODES {
    STOPS = 0, MULTIPLY
};
//defines the range of the Color_Knobs for each mode
static map<int, std::array<int, 2 >> COLOR_KNOB_RANGES = {
    {MATH_MODES::STOPS, std::array<int, 2>{-10, 20}},
    {MATH_MODES::MULTIPLY, std::array<int, 2>{0, 50}},
};
// names to be used in the enumeration knob to describe the mathModes
static const char* const mathModeNames[] = {
    "stops", "multiply", 0
};
//name given to the knob that acts on all layers
static const char* const MASTER_KNOB_NAME = "master";
/**
 * Convenience object for storing, accessing, and calculating color knob values specific to GradeBeauty
 * 
 * Most cg render engines output buffers as additive layers, so the math is extremely simple as it is additive only
 * 
 * To keep the pixel engine as light as possible, the relationship between the knobs is calculated once during
 * construction as a vector of float pointers pointing back to the main private float values
 * 
 * So each knob represents a possible cg layer, has it's own float[3], and a mapping of pointers to other values that could be relevant.
 * 
 * In conjunction with a chosen mathModes enum value:
 *
 *  - calculates the multiply value for a layer name for a given math mode
 *  - calculates if the stored value is default
 *  - stores relevant values, as to keep any logic out of the pixel_engine
 */
class GradeBeautyValueMap {
private:
    map<string, float[3] > m_valueMap;
public:
    map<string, vector<float*>> ptrValueMap;
    map<string, map<int, float>> channelMultipliers;
    vector<Knob*> m_colorKnobs;
    
    //initializes the layer value mapping and interconnect between of layers
    GradeBeautyValueMap() {
        LayerMap layerMapBeautyShading = layerCollection.categorizeLayers(layers::shading, categorizeType::pub);
        m_colorKnobs.reserve(categories::all.size() + 1);
        float* ptrMaster = m_valueMap[MASTER_KNOB_NAME];
        ptrValueMap[MASTER_KNOB_NAME].emplace_back(ptrMaster);

        for (auto iterLayer = layers::all.begin(); iterLayer != layers::all.end(); iterLayer++) {
            ptrValueMap[*iterLayer].emplace_back(m_valueMap[*iterLayer]);
            ptrValueMap[*iterLayer].emplace_back(ptrMaster);
        }

        for (auto iterLayer = layers::shading.begin(); iterLayer != layers::shading.end(); iterLayer++) {
            for (auto iterGlobal = layers::global.begin(); iterGlobal != layers::global.end(); iterGlobal++) {
                if (layerMapBeautyShading.contains(*iterGlobal)) {
                    if (layerMapBeautyShading.isMember(*iterGlobal, *iterLayer)) {
                        ptrValueMap[*iterLayer].emplace_back(m_valueMap[*iterGlobal]);
                    }
                }
            }
        }
        //printf("created value map %p\n", (void*) this);
    }
    //wrapper function used to calculate the result of user knob values, and stores them into this instance's
    //channelMultipliers map. This is to keep the pixel engine light, by not repeating the algorythm for very scanline
    void populateChannelSetKnobMultipliers(const DD::Image::ChannelSet* channels, const int& soloMode, const int& mathMode, const string& soloLayerName, const StrVecType& soloLayerNames) {
        channelMultipliers.clear();
        foreach(z, (*channels)) {
            int chanIdx = colourIndex(z);
            string layerName = getLayerName(z);
            bool relevantLayer = (!soloMode) ? true : isSoloLayer(layerName, soloLayerName, soloLayerNames);
            if (relevantLayer) {
                channelMultipliers[layerName][chanIdx] = getLayerMultiplier(layerName, chanIdx, mathMode);
            }
        }
    }
    // returns a vector of pointers to all float values associated with this layer at a specific color index
    vector<float*> layerValuePointersForIndex(const string& layerName, const int& colorIndex) const {
        vector<float*> outputVector;
        auto it = ptrValueMap.find(layerName);
        int amount = it->second.size();
        outputVector.reserve(amount);
        for (int idx = 0; idx < amount; idx++) {
            outputVector.emplace_back(&it->second[idx][colorIndex]);
        }
        return outputVector;
    }
    
    //returns the pointer specific to this layer
    float* getLayerFloatPointer(const string& knobName) const {
        return ptrValueMap.find(knobName)->second[0]; // the first index if where the layer pointer is.
    }

    // computes, for a given layer name, the total value to multiply the pixels with at a specific color index for any given
    // math mode
    float getLayerMultiplier(const string& layerName, const int& colorIndex, const int& mode) const {
        vector<float*> values = layerValuePointersForIndex(layerName, colorIndex);
        float out = 0.0f;
        if (mode == MATH_MODES::STOPS) {
            for (auto iterV = values.begin(); iterV != values.end(); iterV++) {
                out += *(*iterV);
            }
            out = (out > 0 ? std::pow(2, out) : 1.0f / pow(4, (fabsf(fabsf(out) / 2.0f))));
        } else if (mode == MATH_MODES::MULTIPLY) {
            out = 1.0f;
            for (auto iterV = values.begin(); iterV != values.end(); iterV++) {
                if (iterV != values.begin()) { // skip the actual layer, it multiplies
                    out *= *(*iterV);
                }
            }
            out *= getLayerFloatPointer(layerName)[colorIndex];
        }
        return out;
    }
    bool isDefault(const string& layerName,  const int& mode) const {
        float sum = 0;
        for (int colorIndex = 0; colorIndex < 3; colorIndex++ ){
            sum += getLayerFloatPointer(layerName)[colorIndex];
        }
        bool isDefault = (sum == (float) mode);
        return isDefault;
    }

    bool isSoloLayer(const string& layerName, const string& soloLayerName, const StrVecType& soloLayerNames) const {
        bool contains = false;
        if (soloLayerName == MASTER_KNOB_NAME || soloLayerName == layerName) {
            contains = true;
        } else {
            contains = std::find(begin(soloLayerNames), end(soloLayerNames), layerName) != soloLayerNames.end();
        }
        return contains;
    }
};

class GradeBeauty : public PixelIop {
    int m_mathMode {MATH_MODES::STOPS};
    bool m_beautyDiff {true};
    ChannelSet m_targetLayer  {Mask_RGB};
    bool m_soloMode {false};
    string m_soloLayerName {MASTER_KNOB_NAME};
    StrVecType m_soloLayerNames;
    LayerSetKnobWrapper layerSetKnob;
    GradeBeautyValueMap m_valueMap;
    vector<Knob*>* colorKnobsPtr {&m_valueMap.m_colorKnobs};

    // utility function to create color knobs for this node
    Knob* createColorKnob(Knob_Callback, float*, const string&, const bool&);
    // utility function to set color knob ranges, uses the integer value of  mathModes as the center
    // can reset values with the bool parameter
    void setKnobRanges(const vector<Knob*>*, const int&, const bool&);
    // utility function that handles the visibility of knobs based on the instance's state
    void setKnobVisibility(const vector<Knob*>*);
    // utility function that handles the setting of the knob's default value, uses python, couldn't find a c++ equivalent
    void setKnobDefaultValue(DD::Image::Op* nukeOpPtr);
    //this function simply tests that the private vector of color knob pointers is complete
    bool colorKnobsPopulated() const;
    // this is only run when knob solo mode is enabled, it tests if a layer name should be rendered
    bool isSoloLayer(const string&) const;
    // this calculates the multiply value for layers for the pixel engine.
    void calculateLayerValues(const DD::Image::ChannelSet*, GradeBeautyValueMap&);

public:
    bool pass_transform() const {return true;}
    // this is so that the output in solo mode does not reuse the cache
    void append(Hash&);
    void _validate(bool for_real);
    void in_channels(int, ChannelSet& channels) const;
    void pixel_engine(const Row&, int, int, int, ChannelMask, Row&);
    void knobs(Knob_Callback);
    int knob_changed(Knob*);

    const char* Class() const {
        return CLASS;
    }
    const char* node_help() const {
        return HELP;
    }
    static const Iop::Description description;

    GradeBeauty(Node* node) : PixelIop(node) {  //printf("created GradeBeauty %p\n", (void*) this);
    }
    ~GradeBeauty() {}
    
};

void GradeBeauty::append(Hash& hash) {
    if (m_soloMode) {
        hash.append(m_soloLayerName);  
    }
}
static Iop* build(Node* node) {
    return (new NukeWrapper(new GradeBeauty(node)))->noChannels()->mixLuminance();
}

const Iop::Description GradeBeauty::description(CLASS, "LayerAlchemy/GradeBeauty", build);
void GradeBeauty::in_channels(int input_number, ChannelSet& mask) const {
    mask += (m_targetLayer + *(layerSetKnob.ptrConfiguredChannels) );
}

void GradeBeauty::_validate(bool for_real) {
    copy_info(); // this copies the input info to the output
    ChannelSet inChannels = info_.channels();
    bool valid = layerSetKnob.validateChannels(this, layerCollection, inChannels);

    if (valid) {
        if (layerSetKnob.channelsChanged(inChannels) || (layerSetKnob.categoryChanged())) {
            layerSetKnob.update(layerCollection, inChannels, CategorizeFilterAllBeauty);
            setKnobVisibility(colorKnobsPtr);
        }
        calculateLayerValues(layerSetKnob.ptrConfiguredChannels, m_valueMap);
        set_out_channels(*(layerSetKnob.ptrConfiguredChannels) + m_targetLayer);
        layerSetKnob.setNodeLabel(this);
    }
}

void GradeBeauty::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out) {
    map<int, float*> targetRowPtrIdxMap;
    Row preRow(x, r); // access the initial target layer

    foreach(z, m_targetLayer) {
        int chanIdx = colourIndex(z);
        if (chanIdx >= 3) {
            out.copy(in, z, x, r);
            continue;
        }
        preRow.copy(in, z, x, r);
        targetRowPtrIdxMap[chanIdx] = out.writableConstant(0, z) + x; // zero out the target layer output pixels
        if (m_beautyDiff && !m_soloMode) {
            out.copy(preRow, z, x, r);
        }
    }
    if (aborted()) {return;}
    foreach(z, layerSetKnob.configuredChannelSet()) {
        int chanIdx = colourIndex(z);
        string layerName = getLayerName(z);
        bool relevantLayer = m_valueMap.channelMultipliers.find(layerName) != m_valueMap.channelMultipliers.end();
        if ((targetRowPtrIdxMap.find(chanIdx) == targetRowPtrIdxMap.end()) || (chanIdx >= 3) || !relevantLayer) {
            out.copy(in, z, x, r);
            continue;
        } else {
            float multValue = m_valueMap.channelMultipliers[layerName][chanIdx];
            const float* inPtr = in[z] + x;
            const float* inPtrEnd = inPtr + (r - x);
            float* outPtr = out.writable(z) + x;
            float* btyChanPtr = targetRowPtrIdxMap[chanIdx];
            for (const float* i = inPtr; i != inPtrEnd; i++) {
                if (m_beautyDiff && !m_soloMode) {
                    *btyChanPtr -= *i;
                }
                *outPtr = *i * multValue;
                *btyChanPtr += *outPtr;
                outPtr++;
                btyChanPtr++;
            }
        }
    }
}

void GradeBeauty::knobs(Knob_Callback f) {
    bool colorKnobVectorComplete = colorKnobsPopulated();
    layerSetKnob.createEnumKnob(f);
    createDocumentationButton(f);
    createColorKnobResetButton(f);
    Divider(f, 0); // separates layer set knobs from the rest

    Input_ChannelMask_knob(f, &m_targetLayer, 0, "target layer");
    SetFlags(f, Knob::NO_ALPHA_PULLDOWN);
    ClearFlags(f, Knob::STARTLINE);
    Tooltip(f, "<p>Selects which layer to pre-subtract layers from (if enabled) and add the modified layers to</p>");

    Enumeration_knob(f, &m_mathMode, mathModeNames, "math_type", "math type");
    Tooltip(f, 
            "<p>multiply : this mode acts like a  chain of multiply nodes:</p>"
            "<p>master * all global contributions (if any) * layer</p>"
            "<p><i>since the layer is multiplied last, you can also use this mode to disable the layer</i></p>"
            "<p>stops : this mode acts like a single exposure node for each layer</p> "
            "<p>it adds all contributions and then does the exposure conversion</p>"
            "<p><i>(for example, if master is set to 1.0 and the layer is set to 0.0, this means one stop over)</i></p>");
    SetFlags(f, Knob::ALWAYS_SAVE);

    Bool_knob(f, &m_beautyDiff, "subtract", "subtract input layers from target");
    Tooltip(f,
            "<p>enabled : the additive sum of the chosen layer set is subtracted from the target layer "
            "before recombining with this node's modifications</p>"
            "<p><i>(this means any difference between the target layer "
            "and the render layers is kept in the final output)</i></p>");
    SetFlags(f, Knob::STARTLINE);
    
    Bool_knob(f, &m_soloMode, "solo", "interactive knob contribution preview");
    Tooltip(f,
            "<p>enabled : outputs the additive sum of the layers being affected by the last color knob that was changed</p>"
            "<p><i>(this is meant to help visualize layer set  contributions)</i></p>"
            "<p>IS NOT SAVED, meant as a non persistent convenience helper, it it only renders to the target layer</p>");
    SetFlags(f, Knob::DO_NOT_WRITE);

    Divider(f, 0); // separates master from the rest

    Knob* masterKnob = createColorKnob(f, m_valueMap.getLayerFloatPointer(MASTER_KNOB_NAME), MASTER_KNOB_NAME, true);
    Tooltip(f, "this knob contributes to each layer");
    
    if (!colorKnobVectorComplete) {
        colorKnobsPtr->emplace_back(masterKnob);
    }
    Divider(f, 0); // separates master from the rest

    for (auto iterLayerName = layers::nonShading.begin(); iterLayerName != layers::nonShading.end(); iterLayerName++) {
        Knob* aovKnob = createColorKnob(f, m_valueMap.getLayerFloatPointer(*iterLayerName), *iterLayerName, false);
        Tooltip(f, "applies to this layer or to layers in this layer set");
        if (!colorKnobVectorComplete) {
            colorKnobsPtr->emplace_back(aovKnob);
        }
    }
    BeginClosedGroup(f, "shading_group", "beauty shading layers");
    SetFlags(f, Knob::HIDDEN);

    for (auto iterLayerName = layers::shading.begin(); iterLayerName != layers::shading.end(); iterLayerName++) {
        Knob* aovKnob = createColorKnob(f, m_valueMap.getLayerFloatPointer(*iterLayerName), *iterLayerName, false);
        Tooltip(f, "apply to this layer only");
        if (!colorKnobVectorComplete) {
            colorKnobsPtr->emplace_back(aovKnob);
        }
    }
    EndGroup(f);
    Divider(f, 0); // separates NukeWrapper knobs created after this
    
    // Toolbar
    BeginToolbar(f, "toolbar", "toolbar");
    Link_knob(f, MASTER_KNOB_NAME, "viewerLink", "GradeBeauty master");
    SetFlags(f, Knob::HIDE_ANIMATION_AND_VIEWS | Knob::NO_UNDO | Knob::NO_RERENDER | Knob::STARTLINE);
    Link_knob(f, "reset values", "", "reset values");
    EndToolbar(f);
}

int GradeBeauty::knob_changed(Knob* k) {
    if (k->is("math_type")) {
        setKnobRanges(colorKnobsPtr, m_mathMode, true);
        setKnobDefaultValue(this);
        return 1;
    } else if (k->is("layer_set")) {
        setKnobVisibility(colorKnobsPtr);
        m_soloLayerName = MASTER_KNOB_NAME;
        return 1;
    } else if (k == &Knob::showPanel) {
        setKnobRanges(colorKnobsPtr, m_mathMode, false);
        setKnobVisibility(colorKnobsPtr);
        setKnobDefaultValue(this);
        return 1;
    } else if (k == &Knob::inputChange) { // make sure the UI is correct
        setKnobVisibility(colorKnobsPtr);
        return 1;
    } else if (k->colorKnob()) {
        if (m_soloLayerName != k->name()) {
            m_soloLayerName = k->name();
            m_soloLayerNames = layerCollection.layers[m_soloLayerName]; // returns empty vector if not found
            k->new_nudge_undo();
        }
        return 1;
    }
    return 1;
}
bool GradeBeauty::colorKnobsPopulated() const {
    return  (colorKnobsPtr->size() >= (categories::all.size() + 1));
}

Knob* GradeBeauty::createColorKnob(Knob_Callback f, float* valueStore, const string& name, const bool& visible) {
    const char* knobName = name.c_str();
    Knob* colorKnob = Color_knob(f, valueStore, IRange(0, 1), knobName, knobName);
    SetFlags(f, Knob::LOG_SLIDER);
    SetFlags(f, Knob::NO_COLOR_DROPDOWN);
    return colorKnob;
}

void GradeBeauty::setKnobRanges(const vector<Knob*>* t_colorKnobs, const int& modeValue, const bool& reset) {
    const char* script = (modeValue == 0 ? "{0}" : "{1}"); 
    for (auto iterKnob = t_colorKnobs->begin(); iterKnob != t_colorKnobs->end(); iterKnob++) {
        if (reset) {
            (*iterKnob)->from_script(script); 
        }
        (*iterKnob)->set_range(COLOR_KNOB_RANGES[modeValue][0], COLOR_KNOB_RANGES[modeValue][1], true);
    }
}

void GradeBeauty::setKnobVisibility(const vector<Knob*>* t_colorKnobs) {
    bool isBeautyShading = (layerSetKnob.configuredLayerSetName() == categories::shading);
    StrVecType layers = layerSetKnob.configuredLayerNames();
    StrVecType layersToShow = isBeautyShading ? categories::global : categories::all;

    for (vector<Knob*>::const_iterator iterKnob = t_colorKnobs->begin(); iterKnob != t_colorKnobs->end(); iterKnob++) {
        Knob* colorKnob = *iterKnob;
        string knobName = colorKnob->name();
        bool contains = (
                (knobName == MASTER_KNOB_NAME) || // always visible
                (isBeautyShading && layerSetKnob.contains(knobName)) || // global controls
                ((std::find(begin(layers), end(layers), knobName)) != layers.end())
                );
        if (contains) {
            colorKnob->clear_flag(Knob::DO_NOT_WRITE);
            colorKnob->set_flag(Knob::ALWAYS_SAVE);
            // since hidden knobs can be revealed, make sure the values are correct
            if (m_valueMap.isDefault(colorKnob->name(), 0.0f) && m_mathMode == MATH_MODES::MULTIPLY) {
                colorKnob->from_script("{1}");
            }
        } else {
            colorKnob->clear_flag(Knob::ALWAYS_SAVE);
            colorKnob->set_flag(Knob::DO_NOT_WRITE);
        }
        colorKnob->visible(contains);
    }
    knob("shading_group")->visible(isBeautyShading);
}

void GradeBeauty::setKnobDefaultValue(DD::Image::Op* nukeOpPtr) {
        nukeOpPtr->script_command(DEFAULT_VALUE_PYSCRIPT, true, false);
}

void GradeBeauty::calculateLayerValues(const DD::Image::ChannelSet* channels, GradeBeautyValueMap& valueMap) {
    valueMap.channelMultipliers.clear();
    foreach(z, (*channels)) {
        int chanIdx = colourIndex(z);
        string layerName = getLayerName(z);
        bool relevantLayer = (!m_soloMode) ? true : valueMap.isSoloLayer(layerName, m_soloLayerName, m_soloLayerNames);
        if (relevantLayer) {
           valueMap.channelMultipliers[layerName][chanIdx] = m_valueMap.getLayerMultiplier(layerName, chanIdx, m_mathMode);
        }
    }
}
