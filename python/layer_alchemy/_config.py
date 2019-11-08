"""Python code for collapsing the configuration files into a single yaml file"""

import os
import glob
import yaml


class ConfigError(Exception):
    """Raised when errors occurs in the Configurations."""


def collapse(folder, outputFile):
    """
    Wrapper function that takes a folder, scans for yaml files, accumulates all keys ans values
    and saves the product as outputFile
    Sorts layer configurations, but not channels, as channels have a set order.

    :param folder: the folder to process
    :param outputFile: the absolute path to write to
    :return: loads the outputFile yaml
    :rtype dict
    """
    configFiles = glob.glob(os.path.join(folder, '*.y*ml'))
    configDictList = [_load(config) for config in configFiles if config]
    collapsedConfigs = _add(configDictList, sort=(True if folder.endswith('layers') else False))
    _save(collapsedConfigs, outputFile)
    writtenConfig = _load(outputFile)  # read back, to be sure.
    return writtenConfig


def _load(yamlFile):
    """
    load a yaml file
    :param yamlFile: absolute path to a yaml file
    :return: the loaded yaml file
    :raises ConfigError: config is not a mapping of sets or lists
    """
    config = None
    with open(yamlFile, 'r') as stream:
        config = yaml.load(stream)
    if not config:
        return config
    if not all(isinstance(key, basestring) and isinstance(value, (set, list))
               for key, value in config.items()):
        raise ConfigError('{} config is not a mapping of sets or lists'.format(yamlFile))
    return config


def _save(collapsedConfigs, yamlFile):
    """
    save a dictionary as a yaml file
    :param collapsedConfigs: the final product to write out
    :param yamlFile: the absolute file path to write to
    """
    kwargs = {'width': 1, 'default_flow_style': False, 'indent': 4}
    with open(yamlFile, 'w+') as stream:
        yaml.dump(collapsedConfigs, stream, **kwargs)


def _add(configDictList, sort=False):
    """
    Accumulates a list of dictionaries into one output dictionary
    :param list configDictList: all the config dictionaries to process
    :param bool sort: as an option, soft the values
    :return: a dict that contains all keys and values
    :rtype dict
    """
    func = sorted if sort else list
    outDict = {}
    for config in configDictList:
        if config is not None:  # placeholder files check
            for key, layerSet in config.items():
                if key in outDict:
                    layers = outDict[key]
                    if isinstance(layerSet, set):
                        outDict[key] = layers.union(layerSet)
                    elif isinstance(layerSet, list):
                        for item in layerSet:
                            outDict[key].append(item)
                else:
                    outDict[key] = layerSet
    outDict = {key: func(value) for key, value in outDict.items()}
    return outDict
