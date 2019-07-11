/* 
 * File:   LayerSetTypes.h
 * Author: sjacob
 *
 * Created on November 22, 2018, 6:28 p.m.
 */
#pragma once
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

// Type alias for storing a string vector of layer names
typedef vector<string> StrVecType;
// Type alias for storing layer category names and a vector of layer names
typedef map<string, vector<string> > StrMapType;
