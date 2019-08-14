"""LayerAlchemy callback module"""

import os

import nuke
import nukescripts

import utilities
import constants


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
