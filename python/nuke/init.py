"""LayerAlchemy Nuke init"""
import nuke

import layer_alchemy.constants
import layer_alchemy.callbacks
import layer_alchemy.utilities

if layer_alchemy.utilities.nukeVersionCompatible():
    layer_alchemy.utilities.validateConfigFileEnvironmentVariables()
    nuke.pluginAddPath(layer_alchemy.constants.LAYER_ALCHEMY_ICON_DIR)
    nuke.pluginAddPath(layer_alchemy.utilities.getPluginDirForCurrentNukeVersion())

    # callbacks
    for pluginName in layer_alchemy.constants.LAYER_ALCHEMY_PLUGINS.keys():
        nuke.addKnobChanged(
            lambda: layer_alchemy.callbacks.knobChangedCommon(nuke.thisNode(), nuke.thisKnob()),
            nodeClass=pluginName
        )
