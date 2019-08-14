"""LayerAlchemy Nuke Menu"""

import nuke

import layer_alchemy.constants
import layer_alchemy.utilities

if layer_alchemy.utilities.nukeVersionCompatible():
    toolbar = nuke.menu("Nodes")
    menu = toolbar.addMenu("LayerAlchemy", icon="layer_alchemy.png", index=-1)

    for pluginName, icon in sorted(layer_alchemy.constants.LAYER_ALCHEMY_PLUGINS.items()):
        menu.addCommand(name=pluginName,
                        command="nuke.createNode('{}')".format(pluginName),
                        icon=icon)

    menu.addSeparator()
    menu.addCommand(name="documentation",
                    command=("import layer_alchemy;import nuke;import nukescripts;"
                             "nukescripts.start(layer_alchemy.utilities.getDocumentationIndexPath())"),
                    icon="documentation.png"
                    )

else:
    currentNukeVersion = layer_alchemy.utilities.getNukeVersionString()
    message = "LayerAlchemy : not loading because no installed plugins found for Nuke {version}".format(
        version=currentNukeVersion)
    consoleMessage = "\xE2\x9D\x97 \x1B[31m{message}\033[0m".format(message=message)
    nuke.tprint(consoleMessage)
    if nuke.GUI:
        nuke.tcl('alert "{message}"'.format(message=message))
