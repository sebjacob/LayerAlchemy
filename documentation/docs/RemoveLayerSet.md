# RemoveLayerSet 

!!! info "" 

    RemoveLayerSet provides a simple way to isolate [LayerSets](core.md#layersets) from multichannel streams

![RemoveLayerSet](media/parameters/RemoveLayerSet.png)


## Knob reference


| knob name | type | what it does |
| --------- | ---- | ------------
| operation | enumeration | keep or remove |
| layer_set | enumeration | decides which [LayerSet](core.md#layersets) to use |
| keep_rgba | bool | if you want to keep rgba in all circumstances, enable this |

## Knob value detail

### operation

| math mode | what it does |
| --------- | ------------ |
| copy | _replaces the  target layer with the additive combination of the [LayerSet](core.md#layersets)_
| add | _add the additive combination of the [LayerSet](core.md#layersets) to the target layer_
| remove | _subtract the additive combination of the [LayerSet](core.md#layersets) from the target layer_
