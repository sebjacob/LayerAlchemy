#include "DDImage/Row.h"
#include "DDImage/Iop.h"
#include "DDImage/Knobs.h"

#include "LayerSet.h"
#include "LayerSetKnob.h"

namespace RemoveLayerSet {

using namespace DD::Image;

static const char* const HELP = "Remove of keep channels using LayerSets";

static const char* const enumOperation[] = {
    "remove", "keep", 0
};

class RemoveLayerSet : public Iop {

private:
    LayerAlchemy::LayerSetKnob::LayerSetKnobData m_lsKnobData;
    int m_operation; // 0 = remove, 1 = keep
    bool m_keepRGBA;

protected:
    void _validate(bool for_real);
    void _request(int x, int y, int r, int t, ChannelMask c1, int count);

public:
    virtual void knobs(Knob_Callback);
    static const Description description;
    void engine(int y, int x, int r, ChannelMask c1, Row& out_row);
    const char* Class() const {return description.name;}
    const char* node_help() const {return HELP;}
    RemoveLayerSet(Node* node);
    ~RemoveLayerSet();
    // channel set that contains all channels that are modified by the node
    ChannelSet activeChannelSet() const {return ChannelSet(m_lsKnobData.m_selectedChannels);}

};
RemoveLayerSet::RemoveLayerSet(Node* node) : Iop(node) {
    m_operation = 1;
    m_keepRGBA = true;
}

static Iop* build(Node* node) {
    return new RemoveLayerSet(node);
}

RemoveLayerSet::~RemoveLayerSet() {}

const Iop::Description RemoveLayerSet::description(
    "RemoveLayerSet",
    "LayerAlchemy/RemoveLayerSet",
    build
);

void RemoveLayerSet::_validate(bool for_real) {
    copy_info(); // this copies the input info to the output
    ChannelSet inChannels = info_.channels();
    if (validateLayerSetKnobUpdate(this, m_lsKnobData, LayerAlchemy::layerCollection, inChannels)) {
        updateLayerSetKnob(this, m_lsKnobData, LayerAlchemy::layerCollection, inChannels);
    }
    
    ChannelSet activeChannels = activeChannelSet();
    
    if (m_operation) { // keep
        info_.channels() &= activeChannels;
        set_out_channels(info_.channels());
    } else { //remove
        info_.turn_off(activeChannels);
        set_out_channels(activeChannels);
    }
    if (m_keepRGBA) {
        info_.turn_on(Mask_RGBA);
    }
}

void RemoveLayerSet::_request(int x, int y, int r, int t, ChannelMask c1, int count) {
    input0().request(x, y, r, t, c1, count);
}

void RemoveLayerSet::engine(int y, int x, int r, ChannelMask c1, Row& out_row) {
    out_row.get(input0(), y, x, r, c1);
    return;
}

void RemoveLayerSet::knobs(Knob_Callback f) {
    LayerAlchemy::LayerSetKnob::LayerSetKnob(f, m_lsKnobData);
    LayerAlchemy::Knobs::createDocumentationButton(f);
    Divider(f, 0); // separates layer set knobs from the rest

    Enumeration_knob(f, &m_operation, enumOperation, "operation");
    Bool_knob(f, &m_keepRGBA, "keep_rgba", "keep rgba");
    Tooltip(f, "if enabled, RGBA is always output");
}
} // End namespace RemoveLayerSet