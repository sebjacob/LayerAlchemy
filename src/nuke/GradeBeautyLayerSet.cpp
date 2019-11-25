#include <memory>
#include "math.h"

#include <DDImage/Row.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>

#include "LayerSet.h"
#include "LayerSetKnob.h"

static const char* const HELP =
        "Provides artist friendly controls to manipulate multichannel cg render passes\n\n"
        "GradeBeautyLayerSet will dynamically reconfigure itself depending on : \n\n"
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

// patch for linux alphas because the pow function behaves badly
// for very large or very small exponent values.
static bool LINUX = false;
#ifdef __alpha
LINUX = true;
#endif

enum operationModes {
    ADD = 0, COPY
};

static const char* const operationNames[] = {
    "add", "copy", 0
};


using namespace DD::Image;
using namespace LayerSet;

static const StrVecType all = {
    "beauty_direct_indirect", "beauty_shading_global", "light_group", "beauty_shading"
};
static const CategorizeFilter CategorizeFilterAllBeauty(all, CategorizeFilter::modes::INCLUDE);

class GradeBeautyLayerSet : public PixelIop {

private:
    float blackpoint[4]{0.0f, 0.0f, 0.0f, 0.0f};
    float whitepoint[4]{1.0f, 1.0f, 1.0f, 1.0f};
    float lift[4]{0.0f, 0.0f, 0.0f, 0.0f};
    float gain[4]{1.0f, 1.0f, 1.0f, 1.0f};
    float offset[4]{0.0f, 0.0f, 0.0f, 0.0f};
    float multiply[4]{1.0f, 1.0f, 1.0f, 1.0f};
    float gamma[4]{1.0f, 1.0f, 1.0f, 1.0f};
    bool reverse{false};
    bool clampBlack{true};
    bool clampWhite{false};
    int m_operation{operationModes::ADD};
    LayerSetKnobData m_lsKnobData;
    ChannelSet m_targetLayer{Mask_RGB};
    // intermediate grade algorithm storage
    float A[3]{0.0f, 0.0f, 0.0f};
    float B[3]{0.0f, 0.0f, 0.0f};
    float G[3]{0.0f, 0.0f, 0.0f};

public:
    void knobs(Knob_Callback);
    void _validate(bool for_real);
    bool pass_transform() const {return true;}
    void in_channels(int, ChannelSet& channels) const;
    void pixel_engine(const Row&, int, int, int, ChannelMask, Row&);
    int knob_changed(Knob*);
    const char* Class() const {return description.name;}
    const char* node_help() const {return HELP;}
    static const Iop::Description description;

    float validateGammaValue(const float&);
    // This function calculates and stores the grade algorithm's intermediate calculations
    void precomputeValues();
    // channel set that contains all channels that are modified by the node
    ChannelSet activeChannelSet() const {return ChannelSet(m_targetLayer + m_lsKnobData.m_selectedChannels);}

    GradeBeautyLayerSet(Node* node) : PixelIop(node) { //printf("created GradeBeautyLayerSet %p\n", (void*) this);
        precomputeValues();
    }
    ~GradeBeautyLayerSet();
};


static Op* build(Node* node) {
    return (new NukeWrapper(new GradeBeautyLayerSet(node)))->noChannels();
}

GradeBeautyLayerSet::~GradeBeautyLayerSet() {}

const Iop::Description GradeBeautyLayerSet::description(
    "GradeBeautyLayerSet",
    "LayerAlchemy/GradeBeautyLayerSet",
    build
);

void GradeBeautyLayerSet::in_channels(int input_number, ChannelSet& mask) const {
    mask += activeChannelSet();
}

void GradeBeautyLayerSet::precomputeValues() {
    for (int chanIdx = 0; chanIdx < 3; chanIdx++) {
        float a = whitepoint[chanIdx] - blackpoint[chanIdx];
        a = a ? (gain[chanIdx] - lift[chanIdx]) / a : 10000.0f;
        a *= multiply[chanIdx];
        float b = offset[chanIdx] + lift[chanIdx] - blackpoint[chanIdx] * a;
        float g = validateGammaValue(gamma[chanIdx]);
        A[chanIdx] = a;
        B[chanIdx] = b;
        G[chanIdx] = g;
    }
}

void GradeBeautyLayerSet::_validate(bool for_real) {
    bool changeZero = false;
    precomputeValues();
    for (int chanIdx = 0; chanIdx <= 3; chanIdx++) {
        if (A[chanIdx] != 1 || B[chanIdx] || gamma[chanIdx] != 1.0f) {
            if (B[chanIdx]) {
                changeZero = true;
            }
        }
    }
    info_.black_outside(!changeZero);
    copy_info(); // this copies the input info to the output
    ChannelSet inChannels = info_.channels();
    if (validateLayerSetKnobUpdate(this, m_lsKnobData, layerCollection, inChannels, CategorizeFilterAllBeauty)) {
        updateLayerSetKnob(this, m_lsKnobData, layerCollection, inChannels, CategorizeFilterAllBeauty);
    }
    set_out_channels(activeChannelSet());
}

void GradeBeautyLayerSet::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out) {
    ChannelSet processChans = activeChannelSet();
    map<int, float*> targetRowPtrIdxMap;

    Row aRow(x, r); // store the initial target layer
    aRow.copy(in, processChans, x, r);

    foreach(z, m_targetLayer) {
        int chanIdx = colourIndex(z);
        targetRowPtrIdxMap[chanIdx] = aRow.writableConstant(0.0f, z) + x;
        if (m_operation == operationModes::ADD)
        {
            aRow.copy(in, z, x, r);
        }
    }

    foreach(channel, m_lsKnobData.m_selectedChannels) {
        int chanIdx = colourIndex(channel);
        if (
        (targetRowPtrIdxMap.find(chanIdx) == targetRowPtrIdxMap.end())
        || chanIdx > 2
        || aRow.is_zero(channel)
        || m_targetLayer.contains(channel)) {
            continue;
        }
        float* btyChanPtr = targetRowPtrIdxMap[chanIdx];
//        const float* pArow = aRow[channel] + x;
//        const float* pArowEnd = pArow + (r - x);
//        float* outPtr = aRow.writable(channel) + x;
        float* pArow = aRow.writable(channel) + x;
        float* pArowEnd = pArow + (r - x);

        float* ptrA = &A[chanIdx];
        float* ptrB = &B[chanIdx];
        float* ptrG = &G[chanIdx];

        for (float* i = pArow; i != pArowEnd; i++) {
            float outPixel = *i;
            float btyPixelValue = *btyChanPtr;

            if (m_operation == operationModes::ADD) {
                btyPixelValue -= *i;
            }

            if (!reverse) {
                if (*ptrA != 1.0f || *ptrB) {
                    outPixel = *i * *ptrA + *ptrB;
                    }
                if (clampWhite || clampBlack) {
                    if (outPixel < 0.0f && clampBlack) { // clamp black
                        outPixel = 0.0f;
                    }
                    if (outPixel > 1.0f && clampWhite) { // clamp white
                        outPixel = 1.0f;
                    }
                }
                if (*ptrG <= 0) {
                    if (outPixel < 1.0f) {
                        outPixel = 0.0f;
                    } else if (outPixel > 1.0f) {
                        outPixel = INFINITY;
                    }
                } else if (*ptrG != 1.0f) {
                    float power = 1.0f / *ptrG;
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
                if (*ptrG <= 0) {
                    outPixel = outPixel > 0.0f ? 1.0f : 0.0f;
                }
                if (*ptrG != 1.0f) {
                    if (LINUX & (outPixel <= 1e-6f && *ptrG > 1.0f)) {
                        outPixel = 0.0f;
                    } else if (outPixel < 1.0f) {
                        outPixel = powf(outPixel, *ptrG);
                    } else {
                        outPixel = 1.0f + (outPixel - 1.0f) * *ptrG;
                    }
                }
                // Reverse the linear part:
                if (*ptrA != 1.0f || *ptrB) {
                    float b = *ptrB;
                    float a = *ptrA;
                    if (a) {
                        a = 1 / a;
                    } else {
                        a = 1.0f;
                    }
                    b = -b * a;
                    outPixel = (outPixel * a) + b;
                }
            }
            // apply to pixels
            btyPixelValue += outPixel;
            if (clampWhite || clampBlack) { // clamps
                if (btyPixelValue < 0.0f && clampBlack) { // clamp black
                    btyPixelValue = outPixel = 0.0f;

                } else if (*btyChanPtr > 1.0f && clampWhite) { // clamp white
                    btyPixelValue = outPixel = 1.0f;
                }
            }
            *btyChanPtr = btyPixelValue;
            *i = outPixel;
            pArow++;
            btyChanPtr++;
        }
    }
    out.copy(aRow, processChans, x, r);
}

void GradeBeautyLayerSet::knobs(Knob_Callback f) {
    LayerSet::LayerSetKnob(f, m_lsKnobData);
    createDocumentationButton(f);
    createColorKnobResetButton(f);

    Divider(f, 0); // separates layer set knobs from the rest

    Input_ChannelMask_knob(f, &m_targetLayer, 0, "target layer");
    SetFlags(f, Knob::NO_ALPHA_PULLDOWN);
    Tooltip(f, "<p>Selects which layer to pre-subtract layers from (if enabled) and offset the modified layers to</p>");
    Enumeration_knob(f, &m_operation, operationNames, "output mode");
    Tooltip(f,
            "<p>add : the additive sum of the chosen layer set is subtracted from the target layer "
            "before recombining with this node's modifications</p>"
            "<p><i>(this means any difference between the target layer "
            "and the render layers is kept in the final output)</i></p>"
            "<p>copy: outputs the additive recombination of the layer set to the target layer "
            "and changes are reflected in layers part of the chosen layer set</p>");
    SetFlags(f, Knob::STARTLINE);
    SetFlags(f, Knob::EXPAND_TO_CONTENTS);

    Divider(f, 0); // separates layer set knobs from the rest

    AColor_knob(f, blackpoint, IRange(-1, 1), "blackpoint");
    Tooltip(f, "This color is turned into black");
    AColor_knob(f, whitepoint, IRange(0, 4), "whitepoint");
    Tooltip(f, "This color is turned into white");
    AColor_knob(f, lift, IRange(-1, 1), "lift", "lift");
    Tooltip(f, "Black is turned into this color");
    AColor_knob(f, gain, IRange(0, 4), "gain", "gain");
    Tooltip(f, "White is turned into this color");
    AColor_knob(f, multiply, IRange(0, 4), "multiply");
    Tooltip(f, "Constant to multiply result by");
    AColor_knob(f, offset, IRange(-1, 1), "offset", "offset");
    Tooltip(f, "Constant to offset to result (raises both black & white, unlike lift)");
    AColor_knob(f, gamma, IRange(.2, 5), "gamma");
    Tooltip(f, "Gamma correction applied to final result");
    Newline(f, "  ");
    Bool_knob(f, &reverse, "reverse");
    Tooltip(f, "Invert the math to undo the correction");
    Bool_knob(f, &clampBlack, "clampBlack", "black clamp");
    Tooltip(f, "Output that is less than zero is changed to zero");
    Bool_knob(f, &clampWhite, "clampWhite", "white clamp");
    Tooltip(f, "Output that is greater than 1 is changed to 1");

    Divider(f, 0); // separates NukeWrapper knobs created after this
}

int GradeBeautyLayerSet::knob_changed(Knob* k) {
    return 1;
}

float GradeBeautyLayerSet::validateGammaValue(const float& gammaValue) {
    if (LINUX) {
        if (gammaValue < 0.008f) {
            return 0.0f;
        } else if (gammaValue > 125.0f) {
            return 125.0f;
        }
    }
    return gammaValue;
}
