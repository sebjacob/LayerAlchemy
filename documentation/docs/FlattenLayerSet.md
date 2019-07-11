# FlattenLayerSet 

!!! info "" 

    FlattenLayerSet provides a simple way to merge additive [LayerSets](core.md#layersets)
     from multichannel cg render passes to any single layer

![FlattenLayerSet](media/parameters/FlattenLayerSet.png)

## Knob reference

| knob name | type | what it does |
| --------- | ---- | ------------ |
| operation | enumeration | decides which method combination to use |
| layer_set | enumeration | decides which [LayerSet](core.md#layersets) to use |
| target_layer | enumeration | selects which layer to pre-subtract layers from (if enabled) and add the modified layers to |

## Knob reference in detail

### operation

| math mode | what it does |
| --------- | ------------ |
| copy | _replaces the  target layer with the additive combination of the [LayerSet](core.md#layersets)_
| add | _add the additive combination of the [LayerSet](core.md#layersets) to the target layer_
| remove | _subtract the additive combination of the [LayerSet](core.md#layersets) from the target layer_
