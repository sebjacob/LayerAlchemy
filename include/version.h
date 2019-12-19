#pragma once
#define LAYER_ALCHEMY_VERSION_MAJOR 0
#define LAYER_ALCHEMY_VERSION_MINOR 8
#define LAYER_ALCHEMY_VERSION_PATCH 0

#include <string>

static const std::string LAYER_ALCHEMY_VERSION_STRING =
    std::to_string(LAYER_ALCHEMY_VERSION_MAJOR) + "." +
    std::to_string(LAYER_ALCHEMY_VERSION_MINOR) + "." +
    std::to_string(LAYER_ALCHEMY_VERSION_PATCH);
