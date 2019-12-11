#include <DDImage/Knobs.h>
#include <DDImage/Row.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>

#include "LayerSet.h"
#include "LayerSetKnob.h"

namespace MultiplyLayerSet {

using namespace DD::Image;

static const char* const HELP = "Multiplies multiple channels using LayerSets";

class MultiplyLayerSet : public PixelIop {

private:
    LayerAlchemy::LayerSetKnob::LayerSetKnobData m_lsKnobData;
    int index;
    float m_multValue[4] = {1, 1, 1, 1};
    ChannelSet m_targetLayerSet;

public:
    void knobs(Knob_Callback);
    void _validate(bool);
    bool pass_transform() const {return true;}
    void in_channels(int, ChannelSet&) const;
    void pixel_engine(const Row &in, int y, int x, int r, ChannelMask, Row & out);
    const char* Class() const {return description.name;}
    const char* node_help() const {return HELP;}
    static const Iop::Description description;
    // channel set that contains all channels that are modified by the node
    ChannelSet activeChannelSet() const {return ChannelSet(m_lsKnobData.m_selectedChannels);}

    MultiplyLayerSet(Node* node) : PixelIop(node) {}
    ~MultiplyLayerSet();
};

// register
static Op* build(Node* node) {
    return (new NukeWrapper(new MultiplyLayerSet(node)))->noChannels()->noUnpremult();
}

MultiplyLayerSet::~MultiplyLayerSet() {}

const Iop::Description MultiplyLayerSet::description(
    "MultiplyLayerSet",
    "LayerAlchemy/MultiplyLayerSet",
    build
);

// update layer set knob and gather selectedChannels
void MultiplyLayerSet::_validate(bool for_real) {
    copy_info(); // this copies the input info to the output
    ChannelSet inChannels = info_.channels();
    if (validateLayerSetKnobUpdate(this, m_lsKnobData, LayerAlchemy::layerCollection, inChannels)) {
        updateLayerSetKnob(this, m_lsKnobData, LayerAlchemy::layerCollection, inChannels);
    }
    set_out_channels(activeChannelSet());
}

void MultiplyLayerSet::in_channels(int input, ChannelSet& mask) const {
    // mask is unchanged
}

void MultiplyLayerSet::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out) {

    foreach(z, channels) {
        const float c = m_multValue[colourIndex(z)];
        const float* inptr = in[z] + x;
        const float* END = inptr + (r - x);
        float* outptr = out.writable(z) + x;
        while (inptr < END)
            *outptr++ = *inptr++ * c;
    }
}

void MultiplyLayerSet::knobs(Knob_Callback f) {
    LayerAlchemy::LayerSetKnob::LayerSetKnob(f, m_lsKnobData);
    LayerAlchemy::Knobs::createDocumentationButton(f);
    Divider(f, 0); // separates layer set knobs from the rest
    AColor_knob(f, m_multValue, IRange(0, 4), "value");
}
} // End namespace MultiplyLayerSet