#include <DDImage/Row.h>
#include <DDImage/Knobs.h>
#include <DDImage/PixelIop.h>

#include "LayerSet.h"

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
    ChannelSet targetLayer;
    int m_operation;

public:
    LayerSetKnobWrapper layerSetKnob;
    void in_channels(int input_number, ChannelSet& channels) const;
    void _validate(bool for_real);
    void pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out);
    static const Iop::Description description;
    void knobs(Knob_Callback f);

    const char* Class() const {
        return description.name;
    }

    const char* node_help() const {
        return HELP;
    }
    FlattenLayerSet(Node* node) : PixelIop(node) {
        m_operation = operationModes::COPY;
        targetLayer = Mask_RGB;
    }
};

static Iop* build(Node* node) {
    return (new FlattenLayerSet(node));
}

const Iop::Description FlattenLayerSet::description(
        "FlattenLayerSet",
        "LayerAlchemy/FlattenLayerSet",
        build);

void FlattenLayerSet::in_channels(int input_number, ChannelSet& mask) const {
    mask += *(layerSetKnob.ptrConfiguredChannels) + targetLayer;
}

void FlattenLayerSet::_validate(bool for_real) {
    copy_info(); // this copies the input info to the output
    ChannelSet inChannels = info_.channels();
    bool valid = layerSetKnob.validateChannels(this, layerCollection, inChannels);

    if (!valid) {
        return;
    }

    if (layerSetKnob.channelsChanged(inChannels) || (layerSetKnob.categoryChanged())) {
        layerSetKnob.update(layerCollection, inChannels, layerFilter);
    }
    info_.turn_on(targetLayer);
    layerSetKnob.setNodeLabel(this);
}

void FlattenLayerSet::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out) {
    map<int, float*> targetRowPtrIdxMap;
    Row preRow(x, r); // access the initial target layer

    foreach(z, targetLayer) {
        int chanIdx = colourIndex(z);
        if (chanIdx >= 3) {
            out.copy(in, z, x, r);
            continue;
        }
        preRow.copy(in, z, x, r); // copy the target layer channel
        targetRowPtrIdxMap[chanIdx] = out.writableConstant(0, z) + x;
        if (m_operation != operationModes(COPY)) {
            out.copy(preRow, z, x, r);
        }
    }

    foreach(z, layerSetKnob.configuredChannelSet()) { // visit every channel in LayerSet
        int chanIdx = colourIndex(z);
        if ((targetRowPtrIdxMap.find(chanIdx) == targetRowPtrIdxMap.end()) || (chanIdx >= 3)) {
            out.copy(in, z, x, r);
            continue;
            }
        else {
        const float* inPtr = in[z] + x;
        const float* inPtrEnd = inPtr + (r - x);
        float* btyChanPtr = targetRowPtrIdxMap[chanIdx];
        for (const float* i = inPtr; i != inPtrEnd; i++) {
            if (m_operation == operationModes(REMOVE)) {
                *btyChanPtr -= *i;
            } else {
                *btyChanPtr += *i;
            }
            btyChanPtr++;
            }
        }
    }
}

void FlattenLayerSet::knobs(Knob_Callback f) {
    layerSetKnob.createEnumKnob(f);
    createDocumentationButton(f);
    Divider(f, 0); // separates layer set knobs from the rest

    Enumeration_knob(f, &m_operation, operationNames, "operation", "operation");
    Tooltip(f, "<p>Selects a type of processing</p>"
            "<b>copy</b> : copies the added layers in the layer set to the target layer\n"
            "<b>add</b> : add the added layers in the layer set to the target layer\n"
            "<b>remove</b> : subtracts the added layers in the layer set to the target layer");
    Input_ChannelMask_knob(f, &targetLayer, 0, "target layer");
    Tooltip(f, "<p>Selects which layer to output the processing to</p>");
}
