#include <memory>
#include "math.h"

#include <DDImage/Row.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>

#include "LayerSet.h"
#include "LayerSetKnob.h"
#include "utilities.cpp"

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
    GradeBeautyLayerSet(Node* node);
    ~GradeBeautyLayerSet();
    // channel set that contains all channels that are modified by the node
    ChannelSet activeChannelSet() const;
    // pixel engine function when anything but the target layer is requested to render
    void channelPixelEngine(const Row&, int, int, int, ChannelSet&, Row&);
    // pixel engine functon when the target layer is requested to render
    void beautyPixelEngine(const Row&, int y, int x, int r, ChannelSet&, Row&);
};

GradeBeautyLayerSet::GradeBeautyLayerSet(Node* node) : PixelIop(node)
{
    precomputeValues();
}

static Op* build(Node* node)
{
    return (new NukeWrapper(new GradeBeautyLayerSet(node)))->noChannels()->mixLuminance();
}

GradeBeautyLayerSet::~GradeBeautyLayerSet() {}

const Iop::Description GradeBeautyLayerSet::description(
    "GradeBeautyLayerSet",
    "LayerAlchemy/GradeBeautyLayerSet",
    build
);

ChannelSet GradeBeautyLayerSet::activeChannelSet() const
{
    ChannelSet outChans;
    std::vector<unsigned> targetIndexes;
    foreach(channel, m_targetLayer)
    {
        targetIndexes.emplace_back(colourIndex(channel));
    }

    foreach(channel, ChannelSet(m_targetLayer + m_lsKnobData.m_selectedChannels))
    {
        unsigned chanIdx = colourIndex(channel);
        bool relevant = find(begin(targetIndexes), end(targetIndexes), chanIdx) != targetIndexes.end();
        if (chanIdx <= 2 && relevant)
        {
            outChans += channel;
        }
    }
    return outChans;
}

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
    validateTargetLayerColorIndex(this, m_targetLayer, 0, 2);

    if (validateLayerSetKnobUpdate(this, m_lsKnobData, layerCollection, inChannels, CategorizeFilterAllBeauty)) {
        updateLayerSetKnob(this, m_lsKnobData, layerCollection, inChannels, CategorizeFilterAllBeauty);
    }
    set_out_channels(activeChannelSet());
}
void GradeBeautyLayerSet::channelPixelEngine(const Row& in, int y, int x, int r, ChannelSet& channels, Row& aRow)
{
    map<Channel, float*> aovPtrIdxMap;
    foreach(channel, channels)
    {
        LayerSet::utilities::hard_copy(in, x, r, channel, aRow);
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
void GradeBeautyLayerSet::beautyPixelEngine(const Row& in, int y, int x, int r, ChannelSet& channels, Row& aRow)
{
    ChannelSet bty = m_targetLayer.intersection(channels);
    ChannelSet aovs = m_lsKnobData.m_selectedChannels.intersection(channels);

    map<unsigned, float*> btyPtrIdxMap;
    map<Channel, float*> aovPtrIdxMap;
    map<Channel, const float*> aovInPtrIdxMap;

    foreach(channel, bty) {
        unsigned chanIdx = colourIndex(channel);
        float* rowBtyChan;
        if (m_operation == operationModes::ADD)
        {
            LayerSet::utilities::hard_copy(in, x, r, channel, aRow);
            rowBtyChan = aRow.writable(channel);
        } 
        else
        {
            rowBtyChan = aRow.writableConstant(0.0f, channel);
        }
        btyPtrIdxMap[chanIdx] = rowBtyChan;

    }
    foreach(channel, aovs) {
        aovPtrIdxMap[channel] = aRow.writable(channel);
        aovInPtrIdxMap[channel] = in[channel];
    }

    for (const auto& kvp : btyPtrIdxMap)
    {
        unsigned btyChanIdx = kvp.first;
        float* aRowBty = kvp.second;

        foreach(aov, aovs)
        {
            unsigned aovChanIdx = colourIndex(aov);
            if (btyChanIdx != aovChanIdx)
            {
                continue;
            }

            float* aRowBty = btyPtrIdxMap[aovChanIdx];
            float* aRowAov = aovPtrIdxMap[aov];
            const float* inAov = aovInPtrIdxMap[aov];
            for (int X = x; X < r; X++)
            {
                float aovPixel = aRowAov[X];
                float btyPixel = aRowBty[X];
                if (m_operation == operationModes::ADD)
                {
                    float aovInPixel = inAov[X];
                    btyPixel -= aovInPixel;
                }
                float resultPixel = btyPixel + aovPixel;
                aRowBty[X] = btyPixel + aovPixel;
            }
        }
        // clamp
        if (clampWhite || clampBlack) {
            for (int X = x; X < r; X++)
            {
                float btyPixel = aRowBty[X];

                if (btyPixel < 0.0f && clampBlack)
                {
                    btyPixel = 0.0f;
                }
                else if (btyPixel > 1.0f && clampWhite)
                {
                    btyPixel = 1.0f;
                }
                aRowBty[X] = btyPixel;
            }
        }
    }
}

void GradeBeautyLayerSet::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out) {
    ChannelSet inChannels = ChannelSet(channels);
    ChannelSet activeChannels = activeChannelSet();
    Row aRow(x, r);
    bool isTargetLayer = m_targetLayer.intersection(inChannels).size() == m_targetLayer.size();

    if (isTargetLayer)
    {
        channelPixelEngine(in, y, x, r, activeChannels, aRow);
        beautyPixelEngine(in, y, x, r, activeChannels, aRow);
    }
    else
    {
        channelPixelEngine(in, y, x, r, inChannels, aRow);
    }
    LayerSet::utilities::hard_copy(aRow, x, r, inChannels, out);
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
