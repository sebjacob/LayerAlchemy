# GradeBeautyLayer 

!!! info "" 

    GradeBeautyLayer provides a simple way to specifically grade a cg layer and replace it in the beauty
        
#### Order of operations :
- input layer subtracted from the target layer
- layer is modified
- modified source layer is added to the target layer


![GradeBeautyLayer](media/parameters/GradeBeautyLayer.png)

## Knob reference

| knob name | type | what it does |
| --------- | ---- | ------------
| source_layer | enumeration | layer to to grade |
| target_layer | enumeration | layer to subtract and add the modied source_layer to |
| reset values | button | resets all color knobs to their defaults |

