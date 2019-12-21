"""LayerAlchemy Nuke Menu"""

import nuke

import layer_alchemy.utilities

if layer_alchemy.utilities.nukeVersionCompatible():
    toolbar = nuke.menu("Nodes")
    menu = toolbar.addMenu("LayerAlchemy", icon="layer_alchemy.png", index=-1)

    menu.addCommand(name='GradeBeauty',
                    command="nuke.createNode('GradeBeauty')",
                    icon="GradeBeauty.png"
                   )
    menu.addCommand(name='GradeBeautyLayerSet',
                    command="nuke.createNode('GradeBeautyLayerSet')",
                    icon="GradeBeautyLayerSet.png"
                   )
    menu.addCommand(name='GradeBeautyLayer',
                    command="nuke.createNode('GradeBeautyLayer')",
                    icon="GradeBeautyLayer.png"
                   )
    menu.addSeparator()
    menu.addCommand(name='GradeLayerSet',
                    command="nuke.createNode('GradeLayerSet')",
                    icon="GradeLayerSet.png"
                   )
    menu.addCommand(name='MultiplyLayerSet',
                    command="nuke.createNode('MultiplyLayerSet')",
                    icon="MultiplyLayerSet.png"
                   )
    menu.addSeparator()
    menu.addCommand(name='FlattenLayerSet',
                    command="nuke.createNode('FlattenLayerSet')",
                    icon="FlattenLayerSet.png"
                   )
    menu.addCommand(name='RemoveLayerSet',
                    command="nuke.createNode('RemoveLayerSet')",
                    icon="RemoveLayerSet.png"
                   )

    menu.addSeparator()
    menu.addCommand(name="documentation",
                    command=(
                        "import layer_alchemy.documentation\n"
                        "webview = layer_alchemy.documentation.displayDocumentation(node=None)\n"
                        "if webview:\n"
                        "    webview.show()"),
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
