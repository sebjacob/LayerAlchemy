#include <DDImage/Knobs.h>
#include <DDImage/Row.h>
#include <DDImage/NukeWrapper.h>

#include "LayerSet.h"
#include "LayerSetKnob.h"

namespace GradeLayerSet {

using namespace DD::Image;

const char* const HELP =
        "<p>This is the classic Grade node augmented to use Layer sets, operations are applied "
        "to every layer in the chosen set</p> "
        "<p>Applies a linear ramp followed by a gamma function to each color channel.</p>"
        "<p>  A = multiply * (gain-lift)/(whitepoint-blackpoint)<br>"
        "  B = offset + lift - A*blackpoint<br>"
        "  output = pow(A*input + B, 1/gamma)</p>"
        "The <i>reverse</i> option is also provided so that you can copy-paste this node to "
        "invert the grade. This will do the opposite gamma correction followed by the "
        "opposite linear ramp.";

class GradeLayerSet : public PixelIop {

private:
    LayerAlchemy::LayerSetKnob::LayerSetKnobData m_lsKnobData;
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
    // intermediate grade algorithm storage
    float A[4]{0.0f, 0.0f, 0.0f, 0.0f};
    float B[4]{0.0f, 0.0f, 0.0f, 0.0f};
    float G[4]{0.0f, 0.0f, 0.0f, 0.0f};

public:
    void knobs(Knob_Callback);
    void _validate(bool);
    bool pass_transform() const {return true;}
    void in_channels(int, ChannelSet&) const;
    void pixel_engine(const Row&, int, int, int, ChannelMask, Row&);
    const char* Class() const {return description.name;}
    const char* node_help() const {return HELP;}
    static const Iop::Description description;
    GradeLayerSet(Node* node);
    ~GradeLayerSet();
    // This function calculates and stores the grade algorithm's intermediate calculations
    bool precomputeValues();
    // channel set that contains all channels that are modified by the node
    ChannelSet activeChannelSet() const {return ChannelSet(m_lsKnobData.m_selectedChannels);}
};

GradeLayerSet::GradeLayerSet(Node* node) : PixelIop(node) {}

static Op* build(Node* node)
{
    return (new NukeWrapper(new GradeLayerSet(node)))->noChannels()->mixLuminance();
}
GradeLayerSet::~GradeLayerSet() {}

const Iop::Description GradeLayerSet::description(
    "GradeLayerSet",
    "LayerAlchemy/GradeLayerSet",
    build
);

void GradeLayerSet::in_channels(int input, ChannelSet& mask) const {}

void GradeLayerSet::_validate(bool for_real)
{
    copy_info(); // this copies the input info to the output
    bool changeZero = precomputeValues();
    info_.black_outside(!changeZero);
    ChannelSet inChannels = info_.channels();

    if (validateLayerSetKnobUpdate(this, m_lsKnobData, LayerAlchemy::layerCollection, inChannels)) {
        updateLayerSetKnob(this, m_lsKnobData, LayerAlchemy::layerCollection, inChannels);
    }
    set_out_channels(activeChannelSet());
}

bool GradeLayerSet::precomputeValues() {
    bool changeZero = false;
    for (unsigned int chanIdx = 0; chanIdx < 4; chanIdx++) {
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

void GradeLayerSet::pixel_engine(const Row& in, int y, int x, int r, ChannelMask inChannels, Row& out)
{
    Row aRow(x, r);
    ChannelSet channels = ChannelSet(inChannels);
    LayerAlchemy::Utilities::gradeChannelPixelEngine(in, y, x, r, channels, aRow, A, B, G, reverse, clampBlack, clampWhite);
    LayerAlchemy::Utilities::hard_copy(aRow, x, r, inChannels, out);
}

void GradeLayerSet::knobs(Knob_Callback f)
{
    LayerAlchemy::LayerSetKnob::LayerSetKnob(f, m_lsKnobData);
    LayerAlchemy::Knobs::createDocumentationButton(f);
    LayerAlchemy::Knobs::createColorKnobResetButton(f);
    LayerAlchemy::Knobs::createVersionTextKnob(f);
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
}
} // End namespace GradeLayerSet
