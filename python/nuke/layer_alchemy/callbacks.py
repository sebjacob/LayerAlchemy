"""shared callback function module for LayerAlchemy"""

import os

import nuke
import nukescripts

import layer_alchemy.utilities


def knobChangedCommon(knob):
    """
    common knobChanged function for plugin ins this suite
    :param knob: the nuke knob object
    :type knob: :class:`nuke.Knob`
    """
    if knob.name() == "docButton":
        documentationIndex = layer_alchemy.utilities.getDocumentationIndexPath()
        if not documentationIndex:
            nuke.message("documentation is unavailable")
            return
        pluginDocFileName = "{0}.{1}".format(knob.node().Class(), 'html')
        htmlFile = os.path.join(os.path.dirname(documentationIndex), pluginDocFileName)
        outputPath = htmlFile if os.path.isfile(htmlFile) else documentationIndex
        nukescripts.start(outputPath)
