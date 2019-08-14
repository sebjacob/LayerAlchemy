"""shared constants module for LayerAlchemy"""

import os

LAYER_ALCHEMY_PLUGINS = {  # The name of the plugin and its icon name
    'GradeBeauty': 'GradeBeauty.png',
    'GradeBeautyLayerSet': 'GradeBeautyLayerSet.png',
    'FlattenLayerSet': 'FlattenLayerSet.png',
    'RemoveLayerSet': 'RemoveLayerSet.png',
    'MultiplyLayerSet': 'MultiplyLayerSet.png',
    'GradeLayerSet': 'GradeLayerSet.png',
}

LAYER_ALCHEMY_CONFIGS_DICT = {
    'LAYER_ALCHEMY_LAYER_CONFIG': 'layers.yaml',
    'LAYER_ALCHEMY_CHANNEL_CONFIG': 'channels.yaml'
}

LAYER_ALCHEMY_URL = 'https://github.com/sebjacob/LayerAlchemy'

_thisDir = os.path.dirname(os.path.realpath(__file__))
_layerAlchemyNukeDir = os.path.abspath(os.path.join(_thisDir, '..'))

LAYER_ALCHEMY_PLUGIN_ROOT_DIR = os.path.abspath(
    os.path.join(_layerAlchemyNukeDir, 'plugins')
)
LAYER_ALCHEMY_ICON_DIR = os.path.abspath(
    os.path.join(_layerAlchemyNukeDir, 'icons')
)
LAYER_ALCHEMY_CONFIGS_DIR = os.path.abspath(
    os.path.join(_layerAlchemyNukeDir, '..', 'configs')
)
LAYER_ALCHEMY_CONFIGTESTER_BIN = os.path.abspath(
    os.path.join(_layerAlchemyNukeDir, '..', 'bin', 'ConfigTester')
)
LAYER_ALCHEMY_DOCUMENTATION_DIR = os.path.abspath(
    os.path.join(_layerAlchemyNukeDir, '..', 'documentation')
)
