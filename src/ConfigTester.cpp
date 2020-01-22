/*
 * Simple executable to test if a yaml file can be loaded and a LayerMap object can be constructed
 * usage example: ConfigTester /path/to/config.yaml
 */

#include <fstream>
#include <dirent.h>

#include "argparse.h"

#include "LayerSetCore.h"
#include "version.h"

static const string greenText = "\x1B[92m";
static const string redText = "\x1B[31m";
static const string endColor = "\033[0m";
static const string emojiOk = "\xE2\x9C\x85 ";
static const string emojiMedical = "\xF0\x9F\x98\xB7 ";
static const string emojiError = "\xE2\x9D\x97 ";

static const std::string DESCRIPTION = "Simple executable to validate yaml files";
static const string LAYER_ALCHEMY_PROJECT_URL = "https://github.com/sebjacob/LayerAlchemy";

static const string HEADER =
    "\nConfigTester " + emojiMedical + "\n\n" + DESCRIPTION + "\n\n"
    "LayerAlchemy " + LAYER_ALCHEMY_VERSION_STRING + "\n" +
     LAYER_ALCHEMY_PROJECT_URL + "\n\n"
    "Example usage: \n\nConfigTester --config /path/to/config.yaml\n" 
    "ConfigTester --config /path/to/config1.yaml /path/to/config2.yaml";


void logException(const char* filePath, const std::exception& e)
{
    std::cerr << emojiError << redText << "[ERROR] LayerAlchemy : " << filePath << e.what() << std::endl << endColor;
}

int main(int argc, const char* argv[])
{
    ArgumentParser parser(DESCRIPTION);
    parser.add_argument("--config", "List of layer names to test", true);
    parser.add_argument("--quiet", "disable terminal output, return code only", false);

    try
    {
        parser.parse(argc, argv);
    }
    catch (const ArgumentParser::ArgumentNotFound &ex)
    {
        std::cout << HEADER << std::endl;
        parser.print_help();

        std::cout << ex.what() << std::endl;
        return 0;
    }
    if (parser.is_help())
        return 0;

    auto configs = parser.getv<std::string>("config");
    bool quiet = parser.get<bool>("quiet");

    for(auto it = configs.begin(); it != configs.end(); ++it) 
    {
        auto configFilePath = it->c_str();
        std::ifstream inputFile(configFilePath);

         if (!inputFile || opendir(configFilePath))
         {
             if (!quiet)
             {
                 logException(configFilePath, std::invalid_argument(" is not a file"));
             }
             return 1;
         }
        try 
        {
            LayerMap testLayerMap = LayerMap(loadConfigToMap(configFilePath));
            if (!quiet)
            {
                std::cout << greenText << emojiOk << "LayerAlchemy : valid configuration file " << configFilePath
                << std::endl << endColor;
            }
        } 
        catch (const std::exception& e) 
        {
            if (!quiet)
            {
                logException(configFilePath, e);
            }
            return 1;
        }
    }
    return 0;
}
