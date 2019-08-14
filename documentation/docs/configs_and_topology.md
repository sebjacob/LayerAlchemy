# Customizing config files

!!! warning "knowledge of this is only required if, for example :"
     
     * adding aovs from another renderer
     * adding custom layers and categories in your workflow

The configuration system contains two folders in _/configs_ that contain multiple yaml files

| directory | uses | collapsed target yaml file |
| --------- | ---- | -------------------------- |
|/configs/layers | comprised of yaml files containing sets of layer names | /configs/layers.yaml |
|/configs/channels | list based yaml files, order matters in channels | /configs/channels.yaml |

## private vs public category types (categorizeType) :
| category | uses | note |
| -------- | ---- | ---- |
|private | meant to be used by the core to help classification only | Names all start with the standard underscore nomenclature ex: "_xyz" |
|public | meant to be viewed by the user to populate menus for example | anything but starting with "_" |

## topology (topologyStyle):

!!! warning "usage"
    - The nuke plugins do not use topology at all.
    - Nuke plugins only use layer names.
    - The topology code is intended for use, for example, in an OpenImageIO multipart creation tool.
    - It helps define and enforce channel naming for reliability and safety reasons(bad channels break Nuke scripts) 

Conceptually, a layer is a grouping of channels.
To create channel names in this system topology style is used.
Topology, and all channel related classifiers are defined in the yaml files located in the channels configs
 folder

topology styles are basically mirrored alternative name given to a channel

| topology style | definition | example |
| -------------- | ---------- | ------- |
|lexical | This topology type means that the channel name is an actual word | _the "red" channel for the 'direct_diffuse' layer would be direct_diffuse.red_ |
| exr | the openEXR style with single capital letters like layer.R for red | _direct_diffuse.R_

### Defining a new layer called ('qc.grain', 'qc.edges', 'qc.diff'):

Adding layers is as easy as adding to the existing yaml files, so this example will showcase something a bit more complex.

We will add the ability to have a new type of layer category that has ('.grain', '.edges', '.diff') channels

We'll start with adding the custom channel names :

- create a facility.yaml file in _configs/channels/_

!!! note "add new private categories: called '_qc' and '_qc_exr' to this new yaml file"
            _qc: &facility_qc
                - grain
                - edges
                - diff
            _qc_exr:
                *facility_qc # because in exr style we want it to be the same as the lexical

- final step for topology is registering this in _configs/channels/topology.yaml_:
!!! note "add this to the list of available topologies"

            topology:
                - _alpha
                - _uv
                - _vec3
                - _vec4
                - _xyz
                - _z
                - _qc
            
            exr_topology:
                - _alpha_exr
                - _uv_exr
                - _vec3_exr
                - _vec4_exr
                - _xyz_exr
                - _z_exr
                - _qc_exr


Now that the channels are defined, let's add a layer that'll use it :

!!! note "add the 'qc' layer in the appropriate configs/layers yaml file, example in facility.yaml"

            # this creates a quality control layer set, and adds a layer called "qc" to it
            #
            qualitity_control: !!set
              ? qc
            # this links the qc layer to the _qc topology 
            _qc: !!set
              ? qc


As of writing you need to manually collapse the yaml files using python, this will be improved in the 
future:

```python
layer_alchemy.collapse('path to layer config folder', '/path/to/layers.yaml')
layer_alchemy.collapse('path to channels config folder', '/path/to/channels.yaml')
```

Then rebuild the project (make install)

You can now test everything is ok with [LayerTester](tools.md#LayerTester)

```bash
./LayerTester --layers qc

<LayerSetCore.LayerMap object at 0x7ffee41fd928> 

{
'all' : ('qc', ),
'quality_control' : ('qc', ),
}
```

```bash
./LayerTester --layers qc --topology

Layer names : 'qc'
Topology style : EXR
<LayerSetCore.LayerMap object at 0x7ffee96f0918> 

{
'qc' : ('qc.grain', 'qc.edges', 'qc.diff', ),
}

Topology style : 'LEXICAL'
<LayerSetCore.LayerMap object at 0x7ffee96f0918> 

{
'qc' : ('qc.grain', 'qc.edges', 'qc.diff', ),
}
```