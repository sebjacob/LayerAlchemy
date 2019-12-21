#include "math.h"

#include <DDImage/Row.h>
#include <DDImage/NukeWrapper.h>

#include "LayerSet.h"
#include "LayerSetKnob.h"
#include "GradeBeautyLayerSet.cpp"


namespace GradeBeautyLayer {

const char* const HELP =
        "<p>Grade node for cg multichannel beauty aovs</p> "
        "order of operations is : \n\n"
        "1 - subtract source layer from the target layer\n"
        "2 - perform grade modification to the source layer\n"
        "3 - add the modified source layer to the target layer\n";

static const char* const layerNames[] = {
    " ", 0
};

using namespace DD::Image;

static const StrVecType all = {
    "beauty_direct_indirect", "beauty_shading_global", "light_group", "beauty_shading"
};
static const CategorizeFilter CategorizeFilterAllBeauty(all, CategorizeFilter::modes::INCLUDE);

class GradeBeautyLayer : public PixelIop {

private:
    float blackpoint[3]{0.0f, 0.0f, 0.0f};
    float whitepoint[3]{1.0f, 1.0f, 1.0f};
    float lift[3]{0.0f, 0.0f, 0.0f};
    float gain[3]{1.0f, 1.0f, 1.0f};
    float offset[3]{0.0f, 0.0f, 0.0f};
    float multiply[3]{1.0f, 1.0f, 1.0f};
    float gamma[3]{1.0f, 1.0f, 1.0f};
    bool reverse{false};
    bool clampBlack{true};
    bool clampWhite{false};
    ChannelSet m_targetLayer{Mask_RGB};
    ChannelSet m_sourceLayer{Mask_None};
    ChannelSet m_selectedLayers;

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
    // channel set that contains all channels that are modified by the node
    ChannelSet activeChannelSet() const;
    // pixel engine function when anything but the target layer is requested to render
    void channelPixelEngine(const Row&, int, int, int, ChannelSet&, Row&);
    // pixel engine functon when the target layer is requested to render
    void beautyPixelEngine(const Row&, int y, int x, int r, ChannelSet&, Row&);
    // This function calculates and stores the grade algorithm's intermediate calculations
    bool precomputeValues();
    GradeBeautyLayer(Node* node);
    ~GradeBeautyLayer();
};

GradeBeautyLayer::GradeBeautyLayer(Node* node) : PixelIop(node)
{
    precomputeValues();
}

static Op* build(Node* node)
{
    return (new NukeWrapper(new GradeBeautyLayer(node)))->noChannels()->mixLuminance();
}

GradeBeautyLayer::~GradeBeautyLayer() {}

const Iop::Description GradeBeautyLayer::description(
    "GradeBeautyLayer",
    "LayerAlchemy/GradeBeautyLayer",
    build
);

ChannelSet GradeBeautyLayer::activeChannelSet() const
{
    ChannelSet outChans = ChannelSet(m_targetLayer + m_sourceLayer);
    return outChans;
}

void GradeBeautyLayer::in_channels(int input_number, ChannelSet& mask) const {
    mask += activeChannelSet();
}

bool GradeBeautyLayer::precomputeValues() {
    bool changeZero = false;
    for (unsigned int chanIdx = 0; chanIdx < 3; chanIdx++) {
        float a = whitepoint[chanIdx] - blackpoint[chanIdx];
        a = a ? (gain[chanIdx] - lift[chanIdx]) / a : 10000.0f;
        a *= multiply[chanIdx];
        float b = offset[chanIdx] + lift[chanIdx] - blackpoint[chanIdx] * a;
        float g = LayerAlchemy::Utilities::validateGammaValue(gamma[chanIdx]);
        A[chanIdx] = a;
        B[chanIdx] = b;
        G[chanIdx] = g;
        if (a != 1.0f || b != 0.0f || g != 1.0f)
        {
            if (b)
            {
                changeZero = true;
            }
        }
    }
    return changeZero;
}

void GradeBeautyLayer::_validate(bool for_real) {
    copy_info(); // this copies the input info to the output
    bool changeZero = precomputeValues();
    info_.black_outside(!changeZero);
    ChannelSet inChannels = info_.channels();
    LayerAlchemy::Utilities::validateTargetLayerColorIndex(this, m_targetLayer, 0, 2);
    m_selectedLayers = activeChannelSet();
    set_out_channels(m_selectedLayers);
    info_.turn_on(m_targetLayer);
}
void GradeBeautyLayer::channelPixelEngine(const Row& in, int y, int x, int r, ChannelSet& channels, Row& aRow)
{
    LayerAlchemy::Utilities::gradeChannelPixelEngine(in, y, x, r, channels, aRow, A, B, G, reverse, clampBlack, clampWhite);
}
void GradeBeautyLayer::beautyPixelEngine(const Row& in, int y, int x, int r, ChannelSet& channels, Row& aRow)
{
    ChannelSet bty = m_targetLayer.intersection(channels);
    ChannelSet aovs = m_sourceLayer.intersection(channels);

    map<unsigned, float*> btyPtrIdxMap;
    map<Channel, float*> aovPtrIdxMap;
    map<Channel, const float*> aovInPtrIdxMap;

    foreach(channel, bty) {
        unsigned chanIdx = colourIndex(channel);
        float* rowBtyChan;
        LayerAlchemy::Utilities::hard_copy(in, x, r, channel, aRow);
        rowBtyChan = aRow.writable(channel);
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
                float aovInPixel = inAov[X];
                btyPixel -= aovInPixel;
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

void GradeBeautyLayer::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out) {
    ChannelSet inChannels = ChannelSet(channels);
    ChannelSet activeChannels = m_selectedLayers;
    Row aRow(x, r);
    bool isTargetLayer = m_targetLayer.intersection(inChannels).size() == m_targetLayer.size();

    if (isTargetLayer)
    {
        LayerAlchemy::Utilities::gradeChannelPixelEngine(in, y, x, r, m_sourceLayer, aRow, A, B, G, reverse, clampBlack, clampWhite);
        beautyPixelEngine(in, y, x, r, activeChannels, aRow);
    }
    else
    {
        LayerAlchemy::Utilities::gradeChannelPixelEngine(in, y, x, r, inChannels, aRow, A, B, G, reverse, clampBlack, clampWhite);
    }
    LayerAlchemy::Utilities::hard_copy(aRow, x, r, inChannels, out);
}

void GradeBeautyLayer::knobs(Knob_Callback f) {
    ChannelSet_knob(f, &m_sourceLayer, "channels", "source layer");
    Tooltip(f,
        "<p>Order of operations :</p>"
        "<p>   1 - source_layer <b>subtracted</b> from target_layer</p>"
        "<p>   2 - source_layer is <b>graded</b></p>"
        "<p>   2 - graded source_layer is <b>added</b> to target_layer</p>"
    );
    SetFlags(f, Knob::NO_ALPHA_PULLDOWN);
    SetFlags(f, Knob::NO_CHECKMARKS);

    LayerAlchemy::Knobs::createDocumentationButton(f);
    LayerAlchemy::Knobs::createColorKnobResetButton(f);
    LayerAlchemy::Knobs::createVersionTextKnob(f);
    Divider(f, 0); // separates layer set knobs from the rest

    Input_ChannelMask_knob(f, &m_targetLayer, 0, "target layer");
    SetFlags(f, Knob::NO_ALPHA_PULLDOWN);
    Tooltip(f, "<p>Selects which layer to pre-subtract layers from (if enabled) and offset the modified layers to</p>");
    SetFlags(f, Knob::EXPAND_TO_CONTENTS);

    Divider(f, 0); // separates layer set knobs from the rest

    Color_knob(f, blackpoint, IRange(-1, 1), "blackpoint");
    Tooltip(f, "This color is turned into black");
    Color_knob(f, whitepoint, IRange(0, 4), "whitepoint");
    Tooltip(f, "This color is turned into white");
    Color_knob(f, lift, IRange(-1, 1), "lift", "lift");
    Tooltip(f, "Black is turned into this color");
    Color_knob(f, gain, IRange(0, 4), "gain", "gain");
    Tooltip(f, "White is turned into this color");
    Color_knob(f, multiply, IRange(0, 4), "multiply");
    Tooltip(f, "Constant to multiply result by");
    Color_knob(f, offset, IRange(-1, 1), "offset", "offset");
    Tooltip(f, "Constant to offset to result (raises both black & white, unlike lift)");
    Color_knob(f, gamma, IRange(.2, 5), "gamma");
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

int GradeBeautyLayer::knob_changed(Knob* k) {
    return 1;
}
} // End namespace GradeBeautyLayer
