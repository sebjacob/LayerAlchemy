"""LayerAlchemy Nuke init"""

import os
import subprocess

layerAlchemyNukeRootDir = os.path.dirname(os.path.realpath(__file__))
layerAlchemyConfigsDir = os.path.abspath(os.path.join(layerAlchemyNukeRootDir, '..', 'configs'))
layerAlchemyConfigTesterBin = os.path.abspath(os.path.join(layerAlchemyNukeRootDir, '..', 'bin',
                                                           'ConfigTester'))
layerAlchemyConfigTokens = zip(("LAYER_ALCHEMY_LAYER_CONFIG", "LAYER_ALCHEMY_CHANNEL_CONFIG"),
                               ('layers', 'channels')
                               )


def _validateConfigFile(configFilePath):
    """
    Test a configuration file path to be sure it is usable in the plugin
    Uses a binary included in the project to test a given configuration file, and will raise an exception
    if something is not valid.
    The idea if to fail fast at startup for any configuration file issue.
    :param configFilePath: absolute path to a yaml file
    """
    if not os.path.isfile(configFilePath):
        raise ValueError("the config file is missing")
    subprocess.call([layerAlchemyConfigTesterBin, configFilePath])


for configName, fileNameToken in layerAlchemyConfigTokens:
    configFile = os.environ.get(configName)  # test if a custom configuration is present and validate it
    if configFile:
        _validateConfigFile(configFile)
    else:  # find the included configurations, validate them, and set the required environment variables.
        configFile = os.path.join(layerAlchemyConfigsDir, '{0}.yaml'.format(fileNameToken))
        _validateConfigFile(configFile)
        os.environ[configName] = configFile
