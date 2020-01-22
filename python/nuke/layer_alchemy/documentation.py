"""shared documentation/help module for LayerAlchemy"""

import os

import nuke

import constants
import utilities

if nuke.GUI:
    if nuke.NUKE_VERSION_MAJOR > 10:
        from PySide2.QtWebEngineWidgets import QWebEngineView as qWebview
        from PySide2.QtCore import QUrl
        from PySide2.QtWidgets import QApplication
    else:
        from PySide.QtWebKit import QWebView as qWebview
        from PySide.QtCore import QUrl
        from PySide.QtGui import QApplication


def documentationPath(node=None):
    """
    find an absolute path to the project documentation, or the html file for the chosen node
    :param node: the Nuke node object
    :type node: :class:`nuke.Node`
    :return: absolute html file path
    :rtype: str
    :raises ValueError if absolutely no html file can be found
    """
    documentationIndexPath = utilities.getDocumentationIndexPath()
    htmlFile = documentationIndexPath
    if not documentationIndexPath or not os.path.exists(documentationIndexPath):
        message = 'Local documentation is unavailable, please visit :\n\n<i>{website}</i>'.format(
            website=constants.LAYER_ALCHEMY_URL)
        if nuke.GUI:
            nuke.message(message)
        raise ValueError(message)
    if node:
        pluginDocFileName = '{0}.html'.format(node.Class())
        htmlFile = os.path.join(os.path.dirname(documentationIndexPath), pluginDocFileName)
        if not os.path.isfile(htmlFile):
            htmlFile = documentationIndexPath
    return htmlFile


def documentationWebViewWidget(documentationPath):
    """
    Creates a PySide web view Widget
    :param str documentationPath: the absolute path to the html file to display
    :return: a PySide web view widget set up with the local documentation
    :rtype: PySide2.QtWebEngineWidgets.QWebEngineView
    """
    webView = qWebview()
    webView.setWindowTitle('LayerAlchemy Documentation')
    qUrl = QUrl.fromLocalFile(documentationPath)
    webView.load(qUrl)
    screenGeo = QApplication.desktop().geometry()
    webView.setMinimumHeight(screenGeo.height() / 2)
    center = screenGeo.center()
    webView.move(center - webView.rect().center())
    return webView


def displayDocumentation(node=None):
    """
    Wrapper function to display documentation in Nuke as a PySide2.QtWebEngineWidgets.QWebEngineView
    Due to a bug in Nuke 11, the documentation will be displayed using the web browser
    https://support.foundry.com/hc/en-us/articles/360000148684-Q100379-Importing-PySide2-QtWebEngine-into-Nuke-11
    :param node: the Nuke node object
    :type node: :class:`nuke.Node`
    :return: a PySide web view widget set up with the local documentation
    :rtype: PySide2.QtWebEngineWidgets.QWebEngineView
    """
    htmlFile = documentationPath(node)
    if nuke.env['NukeVersionMajor'] == 11:
        import nukescripts
        nukescripts.start(htmlFile)
    else:
        return documentationWebViewWidget(htmlFile)
