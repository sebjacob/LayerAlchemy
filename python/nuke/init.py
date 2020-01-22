"""LayerAlchemy Nuke init"""

import layer_alchemy.utilities
import layer_alchemy.callbacks

if layer_alchemy.utilities.nukeVersionCompatible():
    layer_alchemy.utilities.validateConfigFileEnvironmentVariables()
    layer_alchemy.utilities.pluginAddPaths()
    layer_alchemy.callbacks.setupCallbacks()
