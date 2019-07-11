"""LayerAlchemy Nuke Menu"""

import os

import nuke

import layer_alchemy

libExtension = nuke.env['PluginExtension']
nukeVersionMajor = nuke.env['NukeVersionMajor']
nukeVersionMinor = nuke.env['NukeVersionMinor']
nukeVersionDirName = '{major}.{minor}'.format(major=nukeVersionMajor, minor=nukeVersionMinor)

layerAlchemyNukeRootDir = os.path.dirname(os.path.realpath(__file__))
layerAlchemyIconDir = os.path.abspath(os.path.join(layerAlchemyNukeRootDir, 'icons'))
layerAlchemyPluginDir = os.path.abspath(os.path.join(layerAlchemyNukeRootDir, 'plugins'))

nuke.pluginAddPath(layerAlchemyIconDir)
nuke.pluginAddPath(layerAlchemyPluginDir)

plugins = {
    'GradeBeauty': 'GradeBeauty.png',
    'GradeBeautyLayerSet': 'GradeBeautyLayerSet.png',
    'FlattenLayerSet': 'FlattenLayerSet.png',
    'RemoveLayerSet': 'RemoveLayerSet.png',
    'MultiplyLayerSet': 'MultiplyLayerSet.png',
    'GradeLayerSet': 'GradeLayerSet.png',
}

menu = nuke.menu('Nodes').addMenu('LayerAlchemy', icon='layer_alchemy.png')

for pluginName, icon in sorted(plugins.items()):
    pluginFile = '{0}.{1}'.format(pluginName, libExtension)
    if os.path.isfile(os.path.join(layerAlchemyPluginDir, pluginFile)):
        menu.addCommand(name=pluginName,
                                            command="nuke.createNode('{}')".format(pluginName),
                                            icon=icon)

        nuke.load('{0}.{1}'.format(pluginName, libExtension))
        nuke.addKnobChanged(lambda: layer_alchemy.callbacks.knobChangedCommon(nuke.thisKnob()),
                            nodeClass=pluginName)

menu.addSeparator()
menu.addCommand(name='documentation',
                command=("import layer_alchemy;import nuke;import nukescripts;"
                         "nukescripts.start(layer_alchemy.utilities.getDocumentationIndexPath())"),
                icon="documentation.png"
                )
