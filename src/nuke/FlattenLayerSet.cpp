#include <DDImage/Row.h>
#include <DDImage/Knobs.h>
#include <DDImage/PixelIop.h>

#include "LayerSet.h"
#include "LayerSetKnob.h"


namespace FlattenLayerSet {

using namespace DD::Image;

static const char* const HELP =
        "<p>This PixelIop manipulates pixel values of a target layer by adding layers"
        " from a layer set in the incoming image stream.</p>"
        ;

// exclude non color layers, makes no sense for this node
static const StrVecType categoryExcludeFilterList = {"non_color", "base_color", "albedo"};
static const CategorizeFilter excludeLayerFilter(categoryExcludeFilterList, CategorizeFilter::modes::EXCLUDE);

static const StrVecType categoryFilterList = {
    "beauty_direct_indirect", "beauty_shading_global", "light_group", "beauty_shading"
};
static const CategorizeFilter layerFilter(categoryFilterList, CategorizeFilter::modes::INCLUDE);

enum operationModes {
    COPY = 0, ADD, REMOVE
};

static const char* const operationNames[] = {
    "copy", "add", "remove", 0
};

class FlattenLayerSet : public PixelIop {

private:
    LayerAlchemy::LayerSetKnob::LayerSetKnobData m_lsKnobData;
    ChannelSet m_targetLayer  {Mask_RGB};
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
    FlattenLayerSet(Node* node);
    ~FlattenLayerSet();
    // channel set that contains all channels that are modified by the node
    ChannelSet activeChannelSet() const;
};

FlattenLayerSet::FlattenLayerSet(Node* node) : PixelIop(node) {
    m_operation = operationModes::COPY;
    m_targetLayer = Mask_RGB;

}

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
    mask += ChannelMask(activeChannelSet());
}

ChannelSet FlattenLayerSet::activeChannelSet() const {
    ChannelSet outChans;
    foreach(channel, ChannelSet(m_targetLayer + m_lsKnobData.m_selectedChannels))
    {
        if (colourIndex(channel) <= 2) {
            outChans += channel;
        }
    }
    return outChans;
}

void FlattenLayerSet::_validate(bool for_real) {
    copy_info(); // this copies the input info to the output
    ChannelSet inChannels = info_.channels();
    LayerAlchemy::Utilities::validateTargetLayerColorIndex(this, m_targetLayer, 0, 2);
    if (validateLayerSetKnobUpdate(this, m_lsKnobData, LayerAlchemy::layerCollection, inChannels, excludeLayerFilter)) {
        updateLayerSetKnob(this, m_lsKnobData, LayerAlchemy::layerCollection, inChannels, excludeLayerFilter);
    }
    set_out_channels(activeChannelSet());
}

void FlattenLayerSet::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out) {
    ChannelSet inChannels = ChannelSet(channels);
    ChannelSet activeChannels = activeChannelSet();
    bool isTargetLayer = m_targetLayer.intersection(inChannels).size() == m_targetLayer.size();
    if (!isTargetLayer)
    {
        out.copy(in, inChannels, x, r);
        return;
    }
    Row aRow(x, r);
    ChannelSet bty = m_targetLayer.intersection(inChannels);
    ChannelSet aovs = activeChannels - bty;

    map<unsigned, float*> btyPtrIdxMap;
    map<Channel, const float*> aovInPtrIdxMap;

    foreach(channel, bty) {
        unsigned chanIdx = colourIndex(channel);
        float* rowBtyChan;
        if (m_operation != operationModes(COPY))
        {
            LayerAlchemy::Utilities::hard_copy(in, x, r, channel, aRow);
            rowBtyChan = aRow.writable(channel);
        } else
        {
            rowBtyChan = aRow.writableConstant(0.0f, channel);
        }
        btyPtrIdxMap[chanIdx] = rowBtyChan;
    }

    LayerAlchemy::Utilities::hard_copy(in, x, r, aovs, aRow);

    for (const auto& kvp : btyPtrIdxMap) 
    {
        unsigned btyChanIdx = kvp.first;
        float* aRowBty = kvp.second;

        foreach(aov, aovs)
        {
            if (btyChanIdx != colourIndex(aov))
            {
                continue;
            }
            const float* inAovChan = aRow[aov];

            for (int X = x; X < r; X++)
            {
                float aovPixel = inAovChan[X];
                float btyPixel = aRowBty[X];

                if (m_operation == operationModes(REMOVE)) {
                    aRowBty[X] -= aovPixel;
                } else {
                    aRowBty[X] += aovPixel;
                }
            }
        }
    }
    LayerAlchemy::Utilities::hard_copy(aRow, x, r, inChannels, out);
}

void FlattenLayerSet::knobs(Knob_Callback f) {
    LayerAlchemy::LayerSetKnob::LayerSetKnob(f, m_lsKnobData);
    LayerAlchemy::Knobs::createDocumentationButton(f);
    Divider(f, 0); // separates layer set knobs from the rest

    Enumeration_knob(f, &m_operation, operationNames, "operation", "operation");
    Tooltip(f, "<p>Selects a type of processing</p>"
            "<b>copy</b> : copies the added layers in the layer set to the target layer\n"
            "<b>add</b> : add the added layers in the layer set to the target layer\n"
            "<b>remove</b> : subtracts the added layers in the layer set to the target layer");
    Input_ChannelMask_knob(f, &m_targetLayer, 0, "target layer");
    Tooltip(f, "<p>Selects which layer to output the processing to</p>");
}
} // End namespace FlattenLayerSet