#include <memory>
#include "math.h"

#include <DDImage/Row.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>

#include "LayerSet.h"
#include "LayerSetKnob.h"

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
static const char* autoLabelScript =
"nuke.thisNode().name() + \"\\n\"+"
"'(' + nuke.thisNode().knob('layer_set').enumName(int(nuke.thisNode().knob('layer_set').getValue())) + ')' ";

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
enum GRADE_BEAUTY_MATH_MODE {
    STOPS = 0, MULTIPLY
};
//defines the range of the Color_Knobs for each mode
static  map<int, std::array<int, 2 >> COLOR_KNOB_RANGES = {
    {GRADE_BEAUTY_MATH_MODE::STOPS, std::array<int, 2>{-10, 20}},
    {GRADE_BEAUTY_MATH_MODE::MULTIPLY, std::array<int, 2>{0, 50}},
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
    map<string, float[3]> m_valueMap;
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
        if (mode == GRADE_BEAUTY_MATH_MODE::STOPS) {
            for (auto iterV = values.begin(); iterV != values.end(); iterV++) {
                out += *(*iterV);
            }
            out = (out > 0 ? std::pow(2, out) : 1.0f / pow(4, (fabsf(fabsf(out) / 2.0f))));
        } else if (mode == GRADE_BEAUTY_MATH_MODE::MULTIPLY) {
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
    void addColorKnob(Knob* knob) {
        m_colorKnobs.emplace_back(knob);
    }
};

class GradeBeauty : public PixelIop {

private:
    int m_mathMode {GRADE_BEAUTY_MATH_MODE::STOPS};
    bool m_beautyDiff {true};
    ChannelSet m_targetLayer  {Mask_RGB};
    LayerSetKnobData m_lsKnobData;
    GradeBeautyValueMap m_valueMap;  
    // utility function to create color knobs for this node
    Knob* createColorKnob(Knob_Callback, float*, const string&, const bool&);
    // utility function to set color knob ranges, uses the integer value of  mathModes as the center
    // can reset values with the bool parameter
    void setKnobRanges(const int&, const bool&);
    // utility function that handles the visibility of knobs based on the instance's state
    void setKnobVisibility();
    // utility function that handles the setting of the knob's default value, uses python, couldn't find a c++ equivalent
    void setKnobDefaultValue(DD::Image::Op* nukeOpPtr);
    //this function simply tests that the private vector of color knob pointers is complete
    bool colorKnobsPopulated() const;
    // this calculates the multiply value for layers for the pixel engine.
    void calculateLayerValues(const DD::Image::ChannelSet&, GradeBeautyValueMap&);
    // channel set that contains all channels that are modified by the node
    ChannelSet activeChannelSet() const {return ChannelSet(m_targetLayer + m_lsKnobData.m_selectedChannels);}

public:
    void knobs(Knob_Callback);
    void _validate(bool);
    bool pass_transform() const {return true;}
    void in_channels(int, ChannelSet&) const;
    void pixel_engine(const Row&, int, int, int, ChannelMask, Row&);
    int knob_changed(Knob*);
    const char* Class() const {return description.name;}
    const char* node_help() const {return HELP;}
    static const Iop::Description description;
    GradeBeauty(Node* node);
    ~GradeBeauty();

};

GradeBeauty::GradeBeauty(Node* node) : PixelIop(node) {
}

static Op* build(Node* node) {
    return (new NukeWrapper(new GradeBeauty(node)))->noChannels()->noUnpremult();
}

GradeBeauty::~GradeBeauty() {}

const Iop::Description GradeBeauty::description(
    "GradeBeauty",
    "LayerAlchemy/GradeBeauty",
    build
);

void GradeBeauty::in_channels(int input_number, ChannelSet& mask) const {
    mask += activeChannelSet();
}

void GradeBeauty::_validate(bool for_real) {
    copy_info(); // this copies the input info to the output
    ChannelSet inChannels = info_.channels();
    if (validateLayerSetKnobUpdate(this, m_lsKnobData, layerCollection, inChannels, CategorizeFilterAllBeauty)) {
        updateLayerSetKnob(this, m_lsKnobData, layerCollection, inChannels, CategorizeFilterAllBeauty);
        setKnobVisibility();
        setKnobRanges(m_mathMode, false);
        setKnobDefaultValue(this);
        _validate(true); // this will refresh the node UI in case the node was blank
    }
    
    calculateLayerValues(m_lsKnobData.m_selectedChannels, m_valueMap);
    set_out_channels(activeChannelSet());
    //printf("knob is %s\n", knob("layer_set")->enumerationKnob()->getSelectedItemString().c_str());
}

void GradeBeauty::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out) {
    ChannelSet processChans = activeChannelSet();
    map<int, float*> targetRowPtrIdxMap;

    Row aRow(x, r);
    aRow.copy(in, processChans, x, r);

    foreach(z, m_targetLayer) {
        int chanIdx = colourIndex(z);
        targetRowPtrIdxMap[chanIdx] = aRow.writableConstant(0.0f, z) + x;
        if (m_beautyDiff)
        {
            aRow.copy(in, z, x, r);
        }
    }

    foreach(z, m_lsKnobData.m_selectedChannels) {
        int chanIdx = colourIndex(z);
        if (
        (targetRowPtrIdxMap.find(chanIdx) == targetRowPtrIdxMap.end())
        || chanIdx > 2
        || aRow.is_zero(z)
        || m_targetLayer.contains(z)) {
            continue;
        }
        string layerName = getLayerName(z);
        float multValue = m_valueMap.channelMultipliers[layerName][chanIdx];

        float* pArow = aRow.writable(z) + x;
        const float* pArowEnd = pArow + (r - x);
        float* btyChanPtr = targetRowPtrIdxMap[chanIdx];

        for (float* i = pArow; i != pArowEnd; i++) {
            if (m_beautyDiff)
            {
               *btyChanPtr -= *i;
            }
            *i *= multValue;
            *btyChanPtr += *i;
            pArow++; btyChanPtr++;
        }
    }
    out.copy(aRow, processChans, x, r);
}

void GradeBeauty::knobs(Knob_Callback f) {
    bool colorKnobVectorComplete = colorKnobsPopulated();
    LayerSet::LayerSetKnob(f, m_lsKnobData);
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

    Bool_knob(f, &m_beautyDiff, "subtract", "subtract layer set from target layer");
    Tooltip(f,
            "<p>enabled : the additive sum of the chosen layer set is subtracted from the target layer "
            "before recombining with this node's modifications</p>"
            "<p><i>(this means any difference between the target layer "
            "and the render layers is kept in the final output)</i></p>");

    Divider(f, 0); // separates master from the rest

    Knob* masterKnob = createColorKnob(f, m_valueMap.getLayerFloatPointer(MASTER_KNOB_NAME), MASTER_KNOB_NAME, true);
    Tooltip(f, "this knob contributes to each layer");
    if (!colorKnobVectorComplete) {
        m_valueMap.addColorKnob(masterKnob);
    }

    Divider(f, 0); // separates master from the rest

    for (auto iterLayerName = layers::nonShading.begin(); iterLayerName != layers::nonShading.end(); iterLayerName++) {
        Knob* aovKnob = createColorKnob(f, m_valueMap.getLayerFloatPointer(*iterLayerName), *iterLayerName, false);
        Tooltip(f, "applies to this layer or to layers in this layer set");
        if (!colorKnobVectorComplete) {
            m_valueMap.addColorKnob(aovKnob);
        }
    }
    BeginClosedGroup(f, "shading_group", "beauty shading layers");
    SetFlags(f, Knob::HIDDEN);

    for (auto iterLayerName = layers::shading.begin(); iterLayerName != layers::shading.end(); iterLayerName++) {
        Knob* aovKnob = createColorKnob(f, m_valueMap.getLayerFloatPointer(*iterLayerName), *iterLayerName, false);
        Tooltip(f, "apply to this layer only");
        if (!colorKnobVectorComplete) {
            m_valueMap.addColorKnob(aovKnob);
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
    // if (!f.makeKnobs()) {
    //     Knob* autoLabelKnob = this->knob("autolabel");
    //     autoLabelKnob->set_flag(Knob::DO_NOT_WRITE);
    //     autoLabelKnob->set_text(autoLabelScript);
    // }
}

int GradeBeauty::knob_changed(Knob* k) {
    if (k->is("math_type")) {
        setKnobRanges(m_mathMode, true);
        setKnobDefaultValue(this);
        return 1;
    }
    return 1;
}

bool GradeBeauty::colorKnobsPopulated() const {
    return  (m_valueMap.m_colorKnobs.size() >= (categories::all.size() + 1));
}

Knob* GradeBeauty::createColorKnob(Knob_Callback f, float* valueStore, const string& name, const bool& visible) {
    const char* knobName = name.c_str();
    Knob* colorKnob = Color_knob(f, valueStore, IRange(COLOR_KNOB_RANGES[this->m_mathMode][0], COLOR_KNOB_RANGES[m_mathMode][1]), knobName, knobName);
    SetFlags(f, Knob::LOG_SLIDER);
    SetFlags(f, Knob::NO_COLOR_DROPDOWN);
    if (f.makeKnobs()) {
        colorKnob->visible(visible);
    }
    return colorKnob;
}

void GradeBeauty::setKnobRanges(const int& modeValue, const bool& reset) {
    const char* script = (modeValue == 0 ? "{0}" : "{1}");
    for (auto iterKnob = m_valueMap.m_colorKnobs.begin(); iterKnob != m_valueMap.m_colorKnobs.end(); iterKnob++) {
        if (reset) {
            (*iterKnob)->from_script(script);
        }
        (*iterKnob)->set_range(COLOR_KNOB_RANGES[modeValue][0], COLOR_KNOB_RANGES[modeValue][1], true);
    }
}

void GradeBeauty::setKnobVisibility() {
    bool isBeautyShading = getLayerSetKnobEnumString(this) == categories::shading;
    LayerMap categorized = layerCollection.categorizeLayers(getLayerNames(m_lsKnobData.m_selectedChannels), categorizeType::pub);

    for (vector<Knob*>::const_iterator iterKnob = m_valueMap.m_colorKnobs.begin(); iterKnob != m_valueMap.m_colorKnobs.end(); iterKnob++) {
        Knob* colorKnob = *iterKnob;
        string knobName = colorKnob->name();
        bool isGlobalLayer = categorized.contains(knobName);
        bool contains = categorized.isMember("all", knobName);

        if (knobName == MASTER_KNOB_NAME)
        {
            contains = true;
        }
        else if (isBeautyShading && isGlobalLayer && categorized.contains(knobName))
        {
            contains = true;
        }

        if (contains) {
            colorKnob->clear_flag(Knob::DO_NOT_WRITE);
            colorKnob->set_flag(Knob::ALWAYS_SAVE);
            // since hidden knobs can be revealed, make sure the values are correct
            if (m_valueMap.isDefault(colorKnob->name(), 1.0f) && m_mathMode == GRADE_BEAUTY_MATH_MODE::MULTIPLY && !colorKnob->is_animated()) {
                colorKnob->from_script("{1}");
            }
        } else {
            colorKnob->clear_flag(Knob::ALWAYS_SAVE);
            colorKnob->set_flag(Knob::DO_NOT_WRITE);
        }
        //printf("setKnobVisibility: knob name is %s and visible is %d is global %d \n", knobName.c_str(), contains, isGlobalLayer);
        colorKnob->visible(contains);
    }
    knob("shading_group")->visible(isBeautyShading);
}

void GradeBeauty::setKnobDefaultValue(DD::Image::Op* nukeOpPtr) {
        nukeOpPtr->script_command(DEFAULT_VALUE_PYSCRIPT, true, false);
}

void GradeBeauty::calculateLayerValues(const DD::Image::ChannelSet& channels, GradeBeautyValueMap& valueMap) {
    valueMap.channelMultipliers.clear();
    foreach(z, channels) {
        int chanIdx = colourIndex(z);
        string layerName = getLayerName(z);
           valueMap.channelMultipliers[layerName][chanIdx] = m_valueMap.getLayerMultiplier(layerName, chanIdx, m_mathMode);
    }
}
