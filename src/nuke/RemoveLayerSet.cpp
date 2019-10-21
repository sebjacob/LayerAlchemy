#include "DDImage/Row.h"
#include "DDImage/Iop.h"
#include "DDImage/Knobs.h"

#include "LayerSet.h"
#include "LayerSetKnob.h"

using namespace DD::Image;
using namespace LayerSet;

static const char* const CLASS = "RemoveLayerSet";
static const char* const HELP = "Remove of keep channels using LayerSets";
static const char* const enumOperation[] = {
    "remove", "keep", 0
};

class RemoveLayerSet : public Iop {

private:
    LayerSetKnobData m_lsKnobData;
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
    if (validateLayerSetKnobUpdate(this, m_lsKnobData, layerCollection, inChannels)) {
        updateLayerSetKnob(this, m_lsKnobData, layerCollection, inChannels);
    }
    setLayerSetNodeLabel(this);

    if (m_operation) { // keep
        info_.channels() &= m_lsKnobData.m_selectedChannels;
        set_out_channels(info_.channels());
    } else { //remove
        info_.turn_off(m_lsKnobData.m_selectedChannels);
        set_out_channels(m_lsKnobData.m_selectedChannels);
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
    Knob* k = LayerSet::LayerSetKnob(f, m_lsKnobData);
    createDocumentationButton(f);
    Divider(f, 0); // separates layer set knobs from the rest

    Enumeration_knob(f, &m_operation, enumOperation, "operation");
    Bool_knob(f, &m_keepRGBA, "keep_rgba", "keep rgba");
    Tooltip(f, "if enabled, RGBA is always output");
}
