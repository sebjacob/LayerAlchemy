"""LayerAlchemy callback module"""

import os

import nuke
import nukescripts

import utilities
import constants


def setupCallbacks():
    """
    Utility function to add all callbacks for plugins in this suite.
    Adds knobChanged and autolabel callbacks
    """
    for pluginName in constants.LAYER_ALCHEMY_PLUGINS.keys():
        nuke.addKnobChanged(
            lambda: knobChangedCommon(nuke.thisNode(), nuke.thisKnob()),
            nodeClass=pluginName
        )
        nuke.addAutolabel(autolabel, nodeClass=pluginName)


def knobChangedCommon(node, knob):
    """
    Common knobChanged function for plugins in this suite
    :param node: the Nuke node object
    :type node: :class:`nuke.Node`
    :param knob: the Nuke knob object
    :type knob: :class:`nuke.Knob`
    """
    if knob.name() == 'docButton':
        documentationIndex = utilities.getDocumentationIndexPath()
        if not documentationIndex:
            message = 'Local documentation is unavailable, please visit :\n\n<i>{website}</i>'.format(
                website=constants.LAYER_ALCHEMY_URL)
            nuke.message(message)
            return
        pluginDocFileName = '{0}.{1}'.format(node.Class(), 'html')
        htmlFile = os.path.join(os.path.dirname(documentationIndex), pluginDocFileName)
        outputPath = htmlFile if os.path.isfile(htmlFile) else documentationIndex
        nukescripts.start(outputPath)


def autolabel():
    """
    Common autolabel function for plugins in this suite
    """

    node = nuke.thisNode()
    nodeName = node.name()
    layerSetKnob = node.knob('layer_set')
    index = int(layerSetKnob.getValue())
    layerSetName = layerSetKnob.enumName(index)
    node.knob('indicators').setValue(_getIndicatorValue(node))
    if layerSetName:
        return '{name}\n({layerSet})'.format(name=nodeName, layerSet=layerSetName)
    else:
        return nodeName


def _getIndicatorValue(node):
    """
    simple function to calculate the indicator value for a node
    :param node: the Nuke node object
    :type node: :class:`nuke.Node`
    :return: the integer indicator value
    :rtype: int
    """
    indicators = 0
    knobs = node.allKnobs()
    mixKnob = node.knob('mix')
    if mixKnob and mixKnob.value() != 1:
        indicators += 16
    if node.clones():
        indicators += 8
    if any(knob.isAnimated() for knob in knobs):
        indicators += 1
    if any(knob.hasExpression() for knob in knobs):
        indicators += 2
    return indicators
