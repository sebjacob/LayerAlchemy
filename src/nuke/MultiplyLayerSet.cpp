#include <DDImage/Knobs.h>
#include <DDImage/Row.h>
#include <DDImage/PixelIop.h>
#include <DDImage/NukeWrapper.h>

#include "LayerSet.h"

using namespace DD::Image;
using namespace LayerSet;

static const char* const CLASS = "MultiplyLayerSet";
static const char* const HELP = "Multiplies multiple channels using LayerSets";

class MultiplyLayerSet : public PixelIop {
private:
    // multiply value storage
    float m_multValue[4] = {1, 1, 1, 1};
    LayerSetKnobWrapper layerSetKnob;

public:

    MultiplyLayerSet(Node* node) : PixelIop(node) {
    }
    ~MultiplyLayerSet();

    virtual void knobs(Knob_Callback);
    static const Iop::Description description;
    const char* Class() const {return description.name;}
    const char* node_help() const {return HELP;}

    void _validate(bool);
    bool pass_transform() const {return true;}
    void in_channels(int input, ChannelSet& mask) const;
    void pixel_engine(const Row &in, int y, int x, int r, ChannelMask, Row & out);
    virtual int knob_changed(Knob* k);
};

// register
static Iop* build(Node* node) {
    return (new NukeWrapper(new MultiplyLayerSet(node)))->noChannels()->mixLuminance();
}
const Iop::Description MultiplyLayerSet::description(CLASS, "LayerAlchemy/MultiplyLayerSet", build);
 
// de-constructor
MultiplyLayerSet::~MultiplyLayerSet() {
}

// update layer set knob and gather selectedChannels
void MultiplyLayerSet::_validate(bool for_real) {
    copy_info(); // this copies the input info to the output
    ChannelSet inChannels = info_.channels();
    bool valid = layerSetKnob.validateChannels(this, layerCollection, inChannels);

    if (!valid) {
        return;
    }

    if (layerSetKnob.channelsChanged(inChannels) || (layerSetKnob.categoryChanged())) {
        layerSetKnob.update(layerCollection, inChannels);
    }
    set_out_channels(layerSetKnob.configuredChannelSet());
    layerSetKnob.setNodeLabel(this);

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
    layerSetKnob.createEnumKnob(f);
    createDocumentationButton(f);
    Divider(f, 0); // separates layer set knobs from the rest
    AColor_knob(f, m_multValue, IRange(0, 4), "value");
}

int MultiplyLayerSet::knob_changed(Knob* k) {
    return 1;
}
