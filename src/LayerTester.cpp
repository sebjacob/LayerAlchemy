/*
 * Simple executable to test the layer categorization
 * This will just print out various methods of categorizing.
 */
#include <iostream>

#include "argparse.h"

#include "LayerSetCore.h"
#include "version.h"

static const std::string emojiSick = "\xF0\x9F\x98\xB7 ";
static const string redText = "\x1B[31m";
static const string endColor = "\033[0m";
static const std::string DESCRIPTION = "Simple executable to test the layer categorization";
static const string LAYER_ALCHEMY_PROJECT_URL = "https://github.com/sebjacob/LayerAlchemy";
static const std::string HEADER =
    "\nLayerTester " + emojiSick + "\n" + DESCRIPTION +
    "\n\nLayerAlchemy " + LAYER_ALCHEMY_VERSION_STRING + "\n" +
     LAYER_ALCHEMY_PROJECT_URL + "\n";

void _printLayerMap(const LayerMap &layerMap, std::string message)
{
    std::cout << message + "\n" + layerMap.toString();
}

void _printLayerMapTopology(LayerCollection *layerCollection, const StrVecType layerNames)
{
    _printLayerMap(
        layerCollection->topology(layerNames, topologyStyle::exr), "Topology style : EXR");
    std::cout << std::endl;
    _printLayerMap(
        layerCollection->topology(layerNames, topologyStyle::lexical), "Topology style : LEXICAL");
}

int main(int argc, const char* argv[])
{
    if ((getenv(CHANNEL_ENV_VAR) == NULL) | (getenv(LAYER_ENV_VAR) == NULL))
    {
        std::cerr << HEADER << std::endl;
        std::cerr << redText << std::endl << "MISSING ENVIRONMENT VARIABLES" << std::endl;
        std::cerr << std::endl << "You need to set environment variables pointing to yaml files for :\n"
        << std::endl << LAYER_ENV_VAR << std::endl << CHANNEL_ENV_VAR << std::endl << endColor << std::endl;
        return 1;
    }
    ArgumentParser parser(DESCRIPTION);
    parser.add_argument("--layers", "List of layer names to test", true);
    parser.add_argument("-c", "--categories", "List of categories to filter", false);
    parser.add_argument("--topology", "outputs topology", false);
    parser.add_argument("--use_private", "test the private categorization", false);

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

    auto layerNames = parser.getv<std::string>("layers");
    auto categories = parser.getv<std::string>("categories");
    bool useTopology = parser.get<bool>("topology");
    bool usePrivate = parser.get<bool>("use_private");
    categorizeType catType = usePrivate ? categorizeType::priv : categorizeType::pub;

    try
    {
        LayerCollection layerCollection;

        std::cout << std::endl;
        if (categories.size() == 0)
        { // if categories are requested, print all permutations
            if (useTopology)
            { // if topology is selected, print all permutations, then exit
                std::cout << "Layer names : '" + parser.get<std::string>("layers") << "'\n";
                _printLayerMapTopology(&layerCollection, layerNames);
                return 0;
            }
            else
            {
                _printLayerMap(layerCollection.categorizeLayers(layerNames, catType), "");
            }
            return 0;
        }
        else
        {
            map<string, CategorizeFilter> filterMapping;
            filterMapping["EXCLUDE"] = CategorizeFilter(categories, CategorizeFilter::EXCLUDE);
            filterMapping["INCLUDE"] = CategorizeFilter(categories, CategorizeFilter::INCLUDE);
            filterMapping["ONLY"] = CategorizeFilter(categories, CategorizeFilter::ONLY);
            for (auto &kvp : filterMapping)
            {
                LayerMap categorizedLayers = layerCollection.categorizeLayers(layerNames, catType, kvp.second);
                std::cout << "-----------------------------------------------" << std::endl
                          << "Layer names : '" + parser.get<std::string>("layers") << "'\n"
                          << "CategorizeFilter : " << kvp.first << std::endl
                          << "Category names : '" << parser.get<std::string>("categories") << "'";

                if (useTopology)
                {
                    _printLayerMapTopology(&layerCollection, categorizedLayers.uniqueLayers());
                }
                else
                {
                    _printLayerMap(categorizedLayers, " ");
                }
            }
            return 0;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "LayerAlchemy ERROR : " << e.what() << std::endl;
        return 1;
    }
}
