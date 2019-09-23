"""shared utilities module for LayerAlchemy"""

import os
import subprocess

import nuke

import constants


def getDocumentationIndexPath():
    """
    Find the absolute path to the documentation's main index.html file
    :return: the absolute path or None if it doesn't exist
    :rtype str
    """
    siteIndex = os.path.join(constants.LAYER_ALCHEMY_DOCUMENTATION_DIR, 'index.html')
    return siteIndex if os.path.isfile(siteIndex) else ''


def getNukeVersionString():
    """
    Utility function to get a formatted 'major.minor' Nuke version string.
    Compiled plugins are installed in a folder name per major/minor
    :return: version string like '11.3'
    :rtype str
    """
    return '{major}.{minor}'.format(major=nuke.env['NukeVersionMajor'], minor=nuke.env['NukeVersionMinor'])


def getPluginDirForCurrentNukeVersion():
    """
    Utility Function to get the directory path relevant to the current nuke version
    :return: absolute directory path to plugin directory
    :rtype str
    """
    return os.path.join(constants.LAYER_ALCHEMY_PLUGIN_ROOT_DIR, getNukeVersionString())


def nukeVersionCompatible():
    """
    Utility Function check if suitable LAYER_ALCHEMY_PLUGINS are compiled for the running nuke version
    :return: true if a folder was found, false otherwise
    :rtype bool
    """
    return getNukeVersionString() in os.listdir(constants.LAYER_ALCHEMY_PLUGIN_ROOT_DIR)


def _validateConfigFile(configFilePath):
    """
    Test a configuration file path to be sure it is usable in the plugin
    Uses a binary included in the project to test a given configuration file, and will raise an exception
    if something is not valid.
    The idea if to fail fast at startup for any configuration file issue.
    :param configFilePath: absolute path to a yaml file
    :raises ValueError if configFilePath is missing or is not a valid yaml file
    """
    if not os.path.isfile(configFilePath):
        raise ValueError('missing configuration file')
    error = subprocess.call([constants.LAYER_ALCHEMY_CONFIGTESTER_BIN, configFilePath])
    if error:
        raise ValueError('invalid configuration file')


def validateConfigFileEnvironmentVariables():
    """
    Tests if all required configurations are valid files as defined in constants.LAYER_ALCHEMY_CONFIGS_DICT.
    If current environment variable defines a custom yaml config, it will validate it.
    If none exists, validate and set it the included default configuration
    """
    for envVarName, baseName in constants.LAYER_ALCHEMY_CONFIGS_DICT.items():
        configFile = os.environ.get(envVarName)  # test if a custom configuration is present and validate it
        if not configFile:
            configFile = os.path.join(constants.LAYER_ALCHEMY_CONFIGS_DIR, baseName)
            os.environ[envVarName] = configFile
        _validateConfigFile(configFile)
