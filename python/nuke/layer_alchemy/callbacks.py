"""LayerAlchemy callback module"""


import nuke

import constants


def setupCallbacks():
    """
    Utility function to add all callbacks for plugins in this suite.
    """
    for pluginName in constants.LAYER_ALCHEMY_PLUGIN_NAMES:
        nuke.addAutolabel(_autolabel, nodeClass=pluginName)
    pass


def _autolabel():
    """
    Common autolabel function for plugins in this suite
    :return: the formatted text to use as a label
    :rtype: str
    """

    node = nuke.thisNode()
    nodeName = node.name()
    text = []
    for knobName in ('layer_set', 'channels', 'maskChannelInput', 'unpremult'):
        knob = node.knob(knobName)
        if knob:
            index = int(knob.getValue())
            value = knob.enumName(index) if isinstance(knob, nuke.Enumeration_Knob) else knob.value()
            if value and value != 'none':
                text.append(value)

    node.knob('indicators').setValue(_getIndicatorValue(node))
    if text:
        return '{name}\n({layers})'.format(name=nodeName, layers=' / '.join(text))
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
    maskKnob = node.knob('maskChannelInput')
    if mixKnob and mixKnob.value() != 1:
        indicators += 16
    if maskKnob and maskKnob.getValue() != 0:
        indicators += 4
    if node.clones():
        indicators += 8
    if any(knob.isAnimated() for knob in knobs):
        indicators += 1
    if any(knob.hasExpression() for knob in knobs):
        indicators += 2
    return indicators
