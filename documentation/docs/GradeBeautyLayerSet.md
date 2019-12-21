# GradeBeautyLayerSet 

!!! info "" 

    GradeBeautyLayerSet provides a simple way to specifically grade multiple cg layers using a [LayerSet](core
    .md#layersets)  
    
    

Image processing math is exactly like the Nuke Grade node except that, you can grade multiple layers at the 
same time

![GradeBeautyLayerSet](media/parameters/GradeBeautyLayerSet.png)

## Knob reference

| knob name | type | what it does |
| --------- | ---- | ------------
| layer_set | enumeration | decides which [LayerSet](core.md#layersets) to use |
| output_mode | enumeration | specifies the type of output, add or copy
| reset values | button | resets all color knobs to their defaults |


## Output Modes

| mode name |  what it does |
| --------- |  ------------ |
| copy | outputs only the modified layers to the target layer (added together)|
| add | this first subtracts all layers from the target layer, then adds each of them back
