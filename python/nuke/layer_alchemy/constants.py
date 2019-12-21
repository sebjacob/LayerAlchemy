"""shared constants module for LayerAlchemy"""

import os

LAYER_ALCHEMY_URL = 'https://github.com/sebjacob/LayerAlchemy'

LAYER_ALCHEMY_PLUGIN_NAMES = [
    'GradeBeauty',
    'GradeBeautyLayerSet',
    'GradeBeautyLayer',
    'GradeLayerSet',
    'MultiplyLayerSet',
    'RemoveLayerSet',
    'FlattenLayerSet'
]

LAYER_ALCHEMY_CONFIGS_DICT = {
    'LAYER_ALCHEMY_LAYER_CONFIG': 'layers.yaml',
    'LAYER_ALCHEMY_CHANNEL_CONFIG': 'channels.yaml'
}

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
