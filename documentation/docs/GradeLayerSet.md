# GradeLayerSet 

!!! info "" 

    GradeLayerSet provides a simple way to grade multiple layers using [LayerSets](core.md#layersets)
    

It's exactly like the Nuke Grade node except that, you can grade multiple layers at the same time

If you are grading cg layers and wish to propagate the changes to the beauty a the same time, have a look at
[GradeBeautyLayerSet](GradeBeautyLayerSet.md) 

![GradeLayerSet](media/parameters/GradeLayerSet.png)

## Knob reference

| knob name | type | what it does |
| --------- | ---- | ------------
| layer_set | enumeration | decides which [LayerSet](core.md#layersets) to use |
| reset values | button | resets all color knobs to their defaults |
