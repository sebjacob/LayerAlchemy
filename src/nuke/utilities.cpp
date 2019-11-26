#include <DDImage/Row.h>

#include "LayerSet.h"

namespace LayerSet
{
namespace utilities
{
float* hard_copy(const DD::Image::Row& fromRow, int x, int r, DD::Image::Channel channel, DD::Image::Row& toRow)
{
    float* outAovValue;
    if (fromRow.is_zero(channel)) 
    {
        float* outAovValue = toRow.writableConstant(0.0f, channel);
    } 
    else
    {
        const float* inAovValue = fromRow[channel];
        float* outAovValue = toRow.writable(channel);
        for (int X = x; X < r; X++) 
        {
            outAovValue[X] = inAovValue[X];
        }
    }
    return outAovValue;
}

void hard_copy(const DD::Image::Row& fromRow, int x, int r, DD::Image::ChannelSet channels, DD::Image::Row& toRow)
{
    foreach(channel, channels)
    {
      hard_copy(fromRow, x, r, channel, toRow);
    }
}

} // utilities
} // LayerSet