#include <DDImage/PixelIop.h>
#include <DDImage/Knobs.h>
#include <DDImage/Row.h>
#include <DDImage/DDMath.h>
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

// patch for linux alphas because the pow function behaves badly
// for very large or very small exponent values.
static bool LINUX = false;
#ifdef __alpha
LINUX = true;
#endif

float validateGammaValue(const float& gammaValue) {
    if (LINUX) {
        if (gammaValue < 0.008f) {
            return 0.0f;
        } else if (gammaValue > 125.0f) {
            return 125.0f;
        }
    }
    return gammaValue;
}

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
    void precomputeValues();
    // channel set that contains all channels that are modified by the node
    ChannelSet activeChannelSet() const {return ChannelSet(m_lsKnobData.m_selectedChannels);}

};
GradeLayerSet::GradeLayerSet(Node* node) : PixelIop(node) {
}

static Op* build(Node* node) {
    return (new NukeWrapper(new GradeLayerSet(node)))->noChannels()->mixLuminance();
}
GradeLayerSet::~GradeLayerSet() {}

const Iop::Description GradeLayerSet::description(
    "GradeLayerSet",
    "LayerAlchemy/GradeLayerSet",
    build
);

void GradeLayerSet::in_channels(int input, ChannelSet& mask) const {
//mask is unchanged
}

void GradeLayerSet::_validate(bool for_real) {
    bool change_zero = false;
    precomputeValues();
    if (change_zero) {
        info_.black_outside(false);
    }
    copy_info(); // this copies the input info to the output
    ChannelSet inChannels = info_.channels();

    if (validateLayerSetKnobUpdate(this, m_lsKnobData, LayerAlchemy::layerCollection, inChannels)) {
        updateLayerSetKnob(this, m_lsKnobData, LayerAlchemy::layerCollection, inChannels);
    }
    set_out_channels(activeChannelSet());
}
void GradeLayerSet::precomputeValues() {
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

void GradeLayerSet::pixel_engine(const Row& in, int y, int x, int r,
                                 ChannelMask inChannels, Row& out) {
    Row aRow(x, r);

    map<Channel, float*> aovPtrIdxMap;
    foreach(channel, inChannels)
    {
        LayerAlchemy::Utilities::hard_copy(in, x, r, channel, aRow);
        aovPtrIdxMap[channel] = aRow.writable(channel);
    }

    foreach(channel, inChannels) {
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
    LayerAlchemy::Utilities::hard_copy(aRow, x, r, inChannels, out);
}

void GradeLayerSet::knobs(Knob_Callback f) {
    LayerAlchemy::LayerSetKnob::LayerSetKnob(f, m_lsKnobData);
    LayerAlchemy::Knobs::createDocumentationButton(f);
    LayerAlchemy::Knobs::createColorKnobResetButton(f);
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
