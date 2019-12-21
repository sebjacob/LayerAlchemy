# Config file tools

## ConfigTester

Simple executable to test if a yaml file can be loaded and a LayerMap object can be constructed

This also runs at Nuke startup to make sure the config files are ok

```bash
./ConfigTester

ConfigTester 😷 

Simple executable to validate yaml files

LayerAlchemy 0.9.0
https://github.com/sebjacob/LayerAlchemy

Example usage: 

ConfigTester --config /path/to/config.yaml
ConfigTester --config /path/to/config1.yaml /path/to/config2.yaml
Usage: ./ConfigTester [options]
Options:
    --config               List of layer names to test (Required)
    --quiet                disable terminal output, return code only
```

```bash
./ConfigTester --config $LAYER_ALCHEMY_LAYER_CONFIG
✅ LayerAlchemy : valid configuration file /path/to/layers.yaml
```

## LayerTester
Simple command line utility to verify how the system classifies layer names.

Useful for verifying custom yaml config files.

It uses the main following environment variables the config files at this time.

set those first :
```bash
export LAYER_ALCHEMY_CHANNEL_CONFIG=$PWD/configs/channels.yaml
export LAYER_ALCHEMY_LAYER_CONFIG=$PWD/configs/layers.yaml
```

```bash
./LayerTester

LayerTester 😷 
Simple executable to test the layer categorization

LayerAlchemy 0.5.1 https://github.com/sebjacob/LayerAlchemy

Usage: ../../build_darwin18/LayerTester [options]
Options:
    --layers               List of layer names to test (Required)
    -c, --categories       List of categories to filter
    --topology             outputs topology       
    --use_private          test the private categorization
Required argument not found: --layers
```

### test public categorization

```bash
./LayerTester --layers P diffuse_direct roto_head diffuse_albedo puz_custom specular_indirect

<LayerSetCore.LayerMap object at 0x7ffee60f48b8> 

{
'albedo' : ('diffuse_albedo', ),
'all' : ('P', 'diffuse_direct', 'roto_head', 'diffuse_albedo', 'puz_custom', 'specular_indirect', ),
'base_color' : ('diffuse_albedo', ),
'beauty_shading' : ('diffuse_direct', 'specular_indirect', ),
'diffuse' : ('diffuse_direct', 'diffuse_albedo', ),
'direct' : ('diffuse_direct', ),
'indirect' : ('specular_indirect', ),
'matte' : ('roto_head', ),
'non_color' : ('P', 'roto_head', 'puz_custom', ),
'p' : ('P', ),
'puz' : ('puz_custom', ),
'roto' : ('roto_head', ),
'specular' : ('specular_indirect', ),
}
```
### test private categorization

```bash
./LayerTester --use_private --layers P diffuse_direct roto_head diffuse_albedo puz_custom specular_indirect 


<LayerSetCore.LayerMap object at 0x7ffedfc3e898> 

{
'_alpha' : ('roto_head', ),
'_asset' : ('roto_head', 'puz_custom', ),
'_prefix' : ('P', 'roto_head', 'puz_custom', ),
'_sanitizable' : ('P', ),
'_step' : ('P', ),
'_vec3' : ('diffuse_direct', 'diffuse_albedo', 'puz_custom', 'specular_indirect', ),
'_xyz' : ('P', ),
'all' : ('P', 'diffuse_direct', 'roto_head', 'diffuse_albedo', 'puz_custom', 'specular_indirect', ),
}
```
### test filtered categorization
Following example take various layers but focusses on _non_color_ and _base_color_ categories

```bash
./LayerTester --layers P diffuse_direct roto_head diffuse_albedo puz_custom specular_indirect -c non_color base_color

-----------------------------------------------
Layer names : 'P diffuse_direct roto_head diffuse_albedo puz_custom specular_indirect'
CategorizeFilter : EXCLUDE
Category names : 'non_color base_color' 
<LayerSetCore.LayerMap object at 0x7ffeefaf1888> 

{
'all' : ('diffuse_direct', 'specular_indirect', ),
'beauty_shading' : ('diffuse_direct', 'specular_indirect', ),
'diffuse' : ('diffuse_direct', ),
'direct' : ('diffuse_direct', ),
'indirect' : ('specular_indirect', ),
'specular' : ('specular_indirect', ),
}
-----------------------------------------------
Layer names : 'P diffuse_direct roto_head diffuse_albedo puz_custom specular_indirect'
CategorizeFilter : INCLUDE
Category names : 'non_color base_color' 
<LayerSetCore.LayerMap object at 0x7ffeefaf1888> 

{
'albedo' : ('diffuse_albedo', ),
'all' : ('P', 'roto_head', 'diffuse_albedo', 'puz_custom', ),
'base_color' : ('diffuse_albedo', ),
'diffuse' : ('diffuse_albedo', ),
'matte' : ('roto_head', ),
'non_color' : ('P', 'roto_head', 'puz_custom', ),
'p' : ('P', ),
'puz' : ('puz_custom', ),
'roto' : ('roto_head', ),
}
-----------------------------------------------
Layer names : 'P diffuse_direct roto_head diffuse_albedo puz_custom specular_indirect'
CategorizeFilter : ONLY
Category names : 'non_color base_color' 
<LayerSetCore.LayerMap object at 0x7ffeefaf1888> 

{
'base_color' : ('diffuse_albedo', ),
'non_color' : ('P', 'roto_head', 'puz_custom', ),
}
```