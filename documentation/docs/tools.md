# Config file tools

# ConfigTester

Simple executable to test if a yaml file can be loaded and a LayerMap object can be constructed

```
ConfigTester /path/to/config.yaml
```

# LayerTester
Simple command line utility to verify how the system classifies layer names.

Can be used to verify custom yaml config files.

the following environment variables must be set to where the config files are:
```
export LAYER_ALCHEMY_CHANNEL_CONFIG=$PWD/configs/channels.yaml
export LAYER_ALCHEMY_LAYER_CONFIG=$PWD/configs/layers.yaml
```

```
./layer_alchemy/bin/LayerTester P diffuse_direct roto_head diffuse_albedo puz_shadingFail specular_indirect
public categorization : 

Contents of this LayerMap at 0x7ffee71b5760 is: 

{
'albedo' : ('diffuse_albedo', ),
'all' : ('P', 'diffuse_direct', 'roto_head', 'diffuse_albedo', 'puz_shadingFail', 'specular_indirect', ),
'base_color' : ('diffuse_albedo', ),
'beauty_shading' : ('diffuse_direct', 'specular_indirect', ),
'diffuse' : ('diffuse_direct', 'diffuse_albedo', ),
'direct' : ('diffuse_direct', ),
'indirect' : ('specular_indirect', ),
'matte' : ('roto_head', ),
'non_color' : ('P', 'roto_head', 'puz_shadingFail', ),
'p' : ('P', ),
'puz' : ('puz_shadingFail', ),
'roto' : ('roto_head', ),
'specular' : ('specular_indirect', ),
}

private categorization : 

Contents of this LayerMap at 0x7ffee71b5780 is: 

{
'_alpha' : ('roto_head', ),
'_asset' : ('roto_head', 'puz_shadingFail', ),
'_prefix' : ('P', 'roto_head', 'puz_shadingFail', ),
'_sanitizable' : ('P', ),
'_step' : ('P', ),
'_vec3' : ('diffuse_direct', 'diffuse_albedo', 'puz_shadingFail', 'specular_indirect', ),
'_xyz' : ('P', ),
'all' : ('P', 'diffuse_direct', 'roto_head', 'diffuse_albedo', 'puz_shadingFail', 'specular_indirect', ),
}

filtered excluded categorization : excludes non_color and base_color categories

Contents of this LayerMap at 0x7ffee71b5868 is: 

{
'all' : ('diffuse_direct', 'specular_indirect', ),
'beauty_shading' : ('diffuse_direct', 'specular_indirect', ),
'diffuse' : ('diffuse_direct', ),
'direct' : ('diffuse_direct', ),
'indirect' : ('specular_indirect', ),
'specular' : ('specular_indirect', ),
}

filtered include categorization : include non_color and base_color categories

Contents of this LayerMap at 0x7ffee71b57a0 is: 

{
'albedo' : ('diffuse_albedo', ),
'all' : ('P', 'roto_head', 'diffuse_albedo', 'puz_shadingFail', ),
'base_color' : ('diffuse_albedo', ),
'diffuse' : ('diffuse_albedo', ),
'matte' : ('roto_head', ),
'non_color' : ('P', 'roto_head', 'puz_shadingFail', ),
'p' : ('P', ),
'puz' : ('puz_shadingFail', ),
'roto' : ('roto_head', ),
}

filtered only categorization : only include non_color and base_color categories

Contents of this LayerMap at 0x7ffee71b57c0 is: 

{
'base_color' : ('diffuse_albedo', ),
'non_color' : ('P', 'roto_head', 'puz_shadingFail', ),
}

exr topology style : 

Contents of this LayerMap at 0x7ffee71b5848 is: 

{
'P' : ('P.X', 'P.Y', 'P.Z', ),
'diffuse_albedo' : ('diffuse_albedo.B', 'diffuse_albedo.G', 'diffuse_albedo.R', ),
'diffuse_direct' : ('diffuse_direct.B', 'diffuse_direct.G', 'diffuse_direct.R', ),
'puz_shadingFail' : ('puz_shadingFail.B', 'puz_shadingFail.G', 'puz_shadingFail.R', ),
'roto_head' : ('roto_head.A', ),
'specular_indirect' : ('specular_indirect.B', 'specular_indirect.G', 'specular_indirect.R', ),
}

lexical topology style: 

Contents of this LayerMap at 0x7ffee71b57f0 is: 

{
'P' : ('P.X', 'P.Y', 'P.Z', ),
'diffuse_albedo' : ('diffuse_albedo.red', 'diffuse_albedo.green', 'diffuse_albedo.blue', ),
'diffuse_direct' : ('diffuse_direct.red', 'diffuse_direct.green', 'diffuse_direct.blue', ),
'puz_shadingFail' : ('puz_shadingFail.red', 'puz_shadingFail.green', 'puz_shadingFail.blue', ),
'roto_head' : ('roto_head.alpha', ),
'specular_indirect' : ('specular_indirect.red', 'specular_indirect.green', 'specular_indirect.blue', ),
}
```