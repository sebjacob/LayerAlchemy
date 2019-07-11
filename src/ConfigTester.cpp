/*
 * Simple executable to test if a yaml file can be loaded and a LayerMap object can be constructed
 * usage example: ConfigTester /path/to/config.yaml
 */

#include <iostream>
#include <fstream>
#include <dirent.h>

#include "LayerSetCore.h"

static const string greenText = "\x1B[92m";
static const string redText = "\x1B[31m";
static const string endColor = "\033[0m";
static const string emojiOk = "\xE2\x9C\x85 ";
static const string emojiMedical = "\xF0\x9F\x98\xB7 ";
static const string emojiError = "\xE2\x9D\x97 ";

int main(int argc, char** argv) {
    if (argc <= 1) {
        string usage =
                "\nConfigTester " + emojiMedical + "\n\n"
                "Simple executable to test if a yaml file can be loaded "
                "and a LayerMap object can be constructed\n\n"
                "Example usage: ConfigTester /path/to/config.yaml\n";
        std::cout << usage << std::endl;
        return 1;
    }
    if (argc >= 3) {
        std::cerr << emojiError << redText << "One config at a time!" << std::endl << endColor;
        return 1;
    }
    string strFilePath = reinterpret_cast<char*> (argv[1]);
    std::ifstream inputFile(strFilePath);
    if (!inputFile || opendir(strFilePath.c_str())) {
        std::cerr << emojiError << redText <<
                "LayerAlchemy ERROR : not a file : " << strFilePath
                << std::endl << endColor;
        return 1;
    }
    try {
        LayerMap testLayerMap = LayerMap(loadConfigToMap(strFilePath));

    } catch (const std::exception& e) {
        std::cerr << emojiError << redText <<
                "LayerAlchemy ERROR : configuration file " << strFilePath << " " << e.what()
                << std::endl << endColor;
        return 1;
    }
    std::cout << greenText << emojiOk <<
            "LayerAlchemy : valid configuration file " << strFilePath
            << std::endl << endColor;
    return 0;
}
