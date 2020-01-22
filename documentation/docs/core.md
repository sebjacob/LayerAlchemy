# General concepts and motivation

## Introduction 

The initial motivation behind the development of this plugin suite 
came about when dealing with ever increasing amounts of layers in production, and the adoption of multipart 
exr workflows.

Compositing wise, the outcome if often: over complicated compositing scripts, because the layer mechanics are generally visible.

To fully understand how this plugin suite works, and leverage it, we must cover some key concepts, 
as they will will be referenced throughout the documentation  


!!! info "The consequence of more layers, and the reliance on richer render passes also means that :" 

    - the layer isolation and recombining requires a lot of nodes, makes the script look more complicated
    - accurate layer management is time taken away from the creative aspect of compositing
    - manual layer management is error prone, at some point, an error/omission will creep in.

!!! info "Python powered Groups or Gizmos do a good job, and I've written them before, but they have their limits:"

    - limited mostly by the callback system to trigger behaviors.  
    - cannot be cloned in Nuke.
    - internally, the algorithm is spread out across multiple Nuke nodes that must be managed.
    - lots python and expression code, workarounds are required.
    - some knobs are c++ only.
    - In general, the fewer the nodes, the faster the comp.

## LayerSets


If a _layer_ is a set of _channels_, a _LayerSet_ is then exactly what you think. a set of layers.

Imagine if you could look at a huge list of channels, classify them, and leverage this in native Nuke plugins.

This is the core of LayerAlchemy.

Let's take the common layer name ***"specular_direct"*** for example :

In this plugin suite, if it is found this layer will be categorized as :

The included commandline tool [LayerTester](tools.md#LayerTester) to visualize the classification :

```bash
./LayerTester --layers specular_direct

<LayerSetCore.LayerMap object at 0x7ffeef40f898> 

{
'all' : ('specular_direct', ),
'beauty_shading' : ('specular_direct', ),
'direct' : ('specular_direct', ),
'specular' : ('specular_direct', ),
}

```


!!! info "we can see here that this single layer name is actually part of 3 [LayerSets](core.md#layersets)"

        - beauty_shading
        - direct
        - specular

!!! note "Any LayerSet knob in this plugin suite will allow you to concentrate on [LayerSets](core.md#layersets) rather than layers."

    this means that if you are making a template, and use nodes in this plugin suite, you won't have to 
    update them (unless something is *horribly* missing)


## Config Files

In this project, [LayerSets](core.md#layersets) are defined in [yaml](https://yaml.org/) files
  
This, to some extent, separates the layer names and the categories ([LayerSets](core.md#layersets)) for the
 c++ code so they are adaptable without recompiling rhe plugins. 

The closest analogy I can think of is OCIO configs and OCIOColorSpace nodes in Nuke.

The idea is to group layer names, so that, in Nuke, what the artist interacts with is the category, nver 
the layers.

_please read [channels and topology](configs_and_topology.md) if you want to know more, or need to tweak them_
