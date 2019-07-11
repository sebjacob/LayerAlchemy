/*
 * Simple executable to test the layer categorization
 * This will just print out various methods of categorizing.
 * Can be improved, obviously
 * usage example: LayerTester direct Z P roto_test
 */
#include <iostream>

#include "LayerSetCore.h"

void LOG(const string& message) {
    std::cout << message << std::endl;
}

int main(int argc, char** argv) {

    if ((getenv(CHANNEL_ENV_VAR) == NULL) | (getenv(LAYER_ENV_VAR) == NULL)) {
        std::cerr  << "\nMISSING ENVIRONMENT VARIABLES" << std::endl;
        std::cerr << "\nYou need to set paths to yaml files in " << LAYER_ENV_VAR << " and "  << CHANNEL_ENV_VAR << " environment variables\n" << std::endl;
        return 1;
    }
    if (argc <= 1) {
        std::cerr << "No layer names provided!" << std::endl;
        return 1;
    }
    StrVecType args;
    args.reserve(argc);
    for (int m = 1; m < argc; m++) {
        string test = (string) argv[m];
        args.emplace_back(test);
    }
    static LayerCollection layerCollection;
    LayerMap PUB = layerCollection.categorizeLayers(args, categorizeType::pub);
    LayerMap PRIV = layerCollection.categorizeLayers(args, categorizeType::priv);
    static const StrVecType categoryFilterList = {"non_color", "base_color"}; //example layer categories
    static const CategorizeFilter layerFilterExclude(categoryFilterList, CategorizeFilter::EXCLUDE);
    static const CategorizeFilter layerFilterInclude(categoryFilterList, CategorizeFilter::INCLUDE);
    static const CategorizeFilter layerFilterOnly(categoryFilterList, CategorizeFilter::ONLY);

    LayerMap FILTERED_EXCLUDE = layerCollection.categorizeLayers(args, categorizeType::pub, layerFilterExclude);
    LayerMap FILTERED_INCLUDE = layerCollection.categorizeLayers(args, categorizeType::pub, layerFilterInclude);
    LayerMap FILTERED_ONLY = layerCollection.categorizeLayers(args, categorizeType::pub, layerFilterOnly);
    
    
    std::cout << "public categorization : " << std::endl;
    LOG(PUB.toString());
    std::cout << "private categorization : " << std::endl;
    LOG(PRIV.toString());
    std::cout << "filtered excluded categorization : excludes non_color and base_color categories" << std::endl;
    LOG(FILTERED_EXCLUDE.toString());
    std::cout << "filtered include categorization : include non_color and base_color categories" << std::endl;
    LOG(FILTERED_INCLUDE.toString());
    std::cout << "filtered only categorization : only include non_color and base_color categories" << std::endl;
    LOG(FILTERED_ONLY.toString());

    
    LayerMap asEXR = layerCollection.topology(args, topologyStyle::exr);
    std::cout << "exr topology style : " << std::endl;
    LOG(asEXR.toString());

    LayerMap asLEXICAL = layerCollection.topology(args, topologyStyle::lexical);
    std::cout << "lexical topology style: " << std::endl;
    LOG(asLEXICAL.toString());
    return 0;
}
