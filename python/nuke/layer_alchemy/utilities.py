"""shared utilities module for LayerAlchemy"""

import os


def getDocumentationIndexPath():
    """
    gets the path to the documentation index.html
    :return: the absolute path or None if it doesn't exist
    """
    thisDir = os.path.dirname(os.path.realpath(__file__))
    layerAlchemyDocumentationDir = os.path.abspath(os.path.join(thisDir, '..', '..', 'documentation'))
    siteIndex = os.path.join(layerAlchemyDocumentationDir, 'index.html')
    return siteIndex if os.path.isfile(siteIndex) else None
