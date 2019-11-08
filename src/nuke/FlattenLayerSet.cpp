#include <DDImage/Row.h>
#include <DDImage/Knobs.h>
#include <DDImage/PixelIop.h>

#include "LayerSet.h"
#include "LayerSetKnob.h"

using namespace DD::Image;
using namespace LayerSet;

// exclude non color layers, makes no sense for this node
//static const StrVecType categoryExcludeFilterList = {"non_color", "base_color", "albedo"};
//static const CategorizeFilter excludeLayerFilter(categoryExcludeFilterList, CategorizeFilter::modes::EXCLUDE);

static const StrVecType categoryFilterList = {
    "beauty_direct_indirect", "beauty_shading_global", "light_group", "beauty_shading"
};
static const CategorizeFilter layerFilter(categoryFilterList, CategorizeFilter::modes::INCLUDE);

static const char* const HELP =
        "<p>This PixelIop manipulates pixel values of a target layer by adding layers"
        " from a layer set in the incoming image stream.</p>"
        ;

enum operationModes {
    COPY = 0, ADD, REMOVE
};

static const char* const operationNames[] = {
    "copy", "add", "remove", 0
};

class FlattenLayerSet : public PixelIop {

private:
    ChannelSet m_targetLayer  {Mask_RGB};
    LayerSetKnobData m_lsKnobData;
    int m_operation;

public:
    void knobs(Knob_Callback);
    void _validate(bool);
    bool pass_transform() const {return true;}
    void in_channels(int, ChannelSet&) const;
    void pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out);
    const char* Class() const {return description.name;}
    const char* node_help() const {return HELP;}
    static const Iop::Description description;
    // channel set that contains all channels that are modified by the node
    ChannelSet activeChannelSet() const {return ChannelSet(m_targetLayer + m_lsKnobData.m_selectedChannels);}

    FlattenLayerSet(Node* node) : PixelIop(node) {
        m_operation = operationModes::COPY;
        m_targetLayer = Mask_RGB;
    }
    ~FlattenLayerSet();

};

static Op* build(Node* node) {
    return (new FlattenLayerSet(node));
}

FlattenLayerSet::~FlattenLayerSet() {}

const Iop::Description FlattenLayerSet::description(
    "FlattenLayerSet",
    "LayerAlchemy/FlattenLayerSet",
    build
);

void FlattenLayerSet::in_channels(int input_number, ChannelSet& mask) const {
    mask += activeChannelSet();
}

void FlattenLayerSet::_validate(bool for_real) {
    copy_info(); // this copies the input info to the output
    ChannelSet inChannels = info_.channels();
    if (validateLayerSetKnobUpdate(this, m_lsKnobData, layerCollection, inChannels)) {
        updateLayerSetKnob(this, m_lsKnobData, layerCollection, inChannels);
    }
    set_out_channels(activeChannelSet());
    setLayerSetNodeLabel(this);
}

void FlattenLayerSet::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out) {
    ChannelSet processChans = activeChannelSet();
    map<int, float*> targetRowPtrIdxMap;

    Row aRow(x, r);
    aRow.copy(in, processChans, x, r);

    foreach(z, m_targetLayer) {
        int chanIdx = colourIndex(z);
        targetRowPtrIdxMap[chanIdx] = aRow.writableConstant(0, z) + x;
        if (m_operation != operationModes(COPY)) {
            aRow.copy(in, z, x, r);
        }
    }

    foreach(z, m_lsKnobData.m_selectedChannels) { // visit every channel in LayerSet
        int chanIdx = colourIndex(z);
        if (
        (targetRowPtrIdxMap.find(chanIdx) == targetRowPtrIdxMap.end())
        || chanIdx > 2
        || aRow.is_zero(z)
        || m_targetLayer.contains(z)) {
            continue;
        }
        const float* pArow = aRow[z] + x;
        const float* pArowEnd = pArow + (r - x);
        float* btyChanPtr = targetRowPtrIdxMap[chanIdx];
        for (const float* i = pArow; i != pArowEnd; i++) {
            if (m_operation == operationModes(REMOVE)) {
                *btyChanPtr -= *i;
            } else {
                *btyChanPtr += *i;
            }
            btyChanPtr++;
        }
    }
    out.copy(aRow, processChans, x, r);
}

void FlattenLayerSet::knobs(Knob_Callback f) {
    LayerSet::LayerSetKnob(f, m_lsKnobData);
    createDocumentationButton(f);
    Divider(f, 0); // separates layer set knobs from the rest

    Enumeration_knob(f, &m_operation, operationNames, "operation", "operation");
    Tooltip(f, "<p>Selects a type of processing</p>"
            "<b>copy</b> : copies the added layers in the layer set to the target layer\n"
            "<b>add</b> : add the added layers in the layer set to the target layer\n"
            "<b>remove</b> : subtracts the added layers in the layer set to the target layer");
    Input_ChannelMask_knob(f, &m_targetLayer, 0, "target layer");
    Tooltip(f, "<p>Selects which layer to output the processing to</p>");
}
