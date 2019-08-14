#pragma once

#include "LayerSetTypes.h"
#include "LayerSetConfig.h"

#define LAYER_ENV_VAR "LAYER_ALCHEMY_LAYER_CONFIG"
#define CHANNEL_ENV_VAR "LAYER_ALCHEMY_CHANNEL_CONFIG"

//Enumeration class to  qualify the type of categories to search for.
 enum class categorizeType {
    /**<b>"private" categories : </b>
     * Categories starting with  "_" in the configuration files.
     * the categories are to be used internally in the code
     */ 
    priv,
    /**<b>"public" categories : </b>
     * Categories that the client sees in applications, or tools leveraging this library
     */
    pub
};

//Enumeration class to  qualify the types of  channel naming.
 enum class topologyStyle {
    /**
     * This topology type means that the channel name is an actual word
     * example : the <b>RED</b> channel for the 'direct_diffuse' layer would be : 
     * <b>direct_diffuse.red</b>
     */
    lexical,
     /**
      * This topology type means that the channel name is openexr style
     * example : the <b>RED</b> channel for the 'direct_diffuse' layer would be :
     * <b>direct_diffuse.R</b>
     */
    exr 
};

/**
 * Convenience class for specifying how to interpret incoming channels/layers
 */
class CategorizeFilter {
public:
    enum modes {
        INCLUDE = 0, EXCLUDE, ONLY
    };
    CategorizeFilter();
    CategorizeFilter(const StrVecType&, int);
    int filterMode;
    StrVecType categories;
};

/**
 * Base class for manipulating mappings.
 */
class LayerMap {
public:
    //data stored publicly, for now.
    StrMapType strMap;
    LayerMap();
    LayerMap(const LayerMap&);
    LayerMap(const StrMapType&);
    LayerMap(const string);
    virtual ~LayerMap();

    StrVecType operator[](const StrVecType&);
    StrVecType operator[](const string);

    //adds to category a layer
    void add(const string, const string);
    //returns the  category names in this LayerMap
    const StrVecType categories() const;
    //returns the  category names for a given layer name
    const StrVecType categories(string&) const;
    //Get a vector of category names for each category type
    const StrVecType categoriesByType(const categorizeType&) const;
    //test if a category contains an element
    bool isMember(const string&, const string&) const;
    //test if a category contains an element
    bool contains(const string&) const;
    //test if a category contains a multiple elements
    bool contains(const StrVecType&) const;
    //returns a vector of unique layer names in this LayerMap
    const StrVecType uniqueLayers() const;
    //returns the contents of the object in string form
    string toString() const;
};

/**
 * Object used to store layer configurations, categorize layer names, or create full channel names (topology)
 * It stores both channel and layer LayerMap objects, and the contents are loaded from the defined environment variables
 */
class LayerCollection {

private:
    // takes a given layer name and returns a base category prefix or and empty string otherwise
    string dePrefix(const string) const;

public:
    // default constructor
    LayerCollection();
    // returns a LayerMap of categorized items
    LayerMap categorizeLayers(const StrVecType&, const categorizeType&) const;
    // returns a LayerMap of categorized items, but filtered with a CategorizeFilter
    LayerMap categorizeLayers(const StrVecType&, const categorizeType&, const CategorizeFilter& catFilter) const;
    //The notion of topology is basically adding, for example ".red" to a layer name based on a topologyStyle.
    LayerMap topology(const StrVecType&, const topologyStyle&);

    // houses the map to channel configurations
    LayerMap channels;
    // houses the map to layer configurations
    LayerMap layers;
    //destructor
    virtual ~LayerCollection();
};

//Various useful functions
 namespace utilities {
    string getLayerFromChannel(const string&);
    StrVecType applyChannelNames(const string&, const StrVecType&);
} // utilities
 