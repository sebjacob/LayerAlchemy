#include <DDImage/Row.h>
#include <DDImage/Knobs.h>
#include <DDImage/PixelIop.h>

#include "LayerSet.h"
#include "LayerSetKnob.h"
#include "utilities.cpp"


using namespace DD::Image;
using namespace LayerSet;


// exclude non color layers, makes no sense for this node
static const StrVecType categoryExcludeFilterList = {"non_color", "base_color", "albedo"};
static const CategorizeFilter excludeLayerFilter(categoryExcludeFilterList, CategorizeFilter::modes::EXCLUDE);

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
    validateTargetLayerColorIndex(this, m_targetLayer, 0, 2);
    if (validateLayerSetKnobUpdate(this, m_lsKnobData, layerCollection, inChannels, excludeLayerFilter)) {
        updateLayerSetKnob(this, m_lsKnobData, layerCollection, inChannels, excludeLayerFilter);
    }
    set_out_channels(activeChannelSet());
}

void FlattenLayerSet::pixel_engine(const Row& in, int y, int x, int r, ChannelMask channels, Row& out) {
    ChannelSet inChannels = ChannelSet(channels);
    ChannelSet activeChannels = activeChannelSet();

    bool isTargetLayer = m_targetLayer.intersection(inChannels).size() == m_targetLayer.size();
    if (!isTargetLayer) {
        out.copy(in, channels, x, r);
        return;
    }

    Row aRow(x, r);
    ChannelSet bty = m_targetLayer.intersection(channels);
    ChannelSet aovs = activeChannels - bty;

    map<unsigned, float*> btyPtrIdxMap;

    foreach(channel, bty) 
    {
        unsigned chanIdx = colourIndex(channel);
        btyPtrIdxMap[chanIdx] = aRow.writableConstant(0.0f, channel);
    }

    if (m_operation != operationModes(COPY))
    {
        LayerSet::utilities::hard_copy(in, x, r, bty, aRow);
    }

    for (const auto& kvp : btyPtrIdxMap) 
    {
        unsigned btyChanIdx = kvp.first;
        float* aRowBty = kvp.second;

        foreach(aov, aovs)
        {
            if (in.is_zero(aov) || btyChanIdx != colourIndex(aov))
            {
                continue;
            }
            const float* inAovChan = in[aov];
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
    LayerSet::utilities::hard_copy(aRow, x, r, bty, out);
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
