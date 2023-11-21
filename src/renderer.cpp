#include "renderer.h"

#include <vector>

//! Bresenham’s line drawing algorithm
void line(const ivec2& p1, const ivec2& p2, TGAImage& image, const TGAColor& color)
{
    bool steep = false;
    int x1 = p1[0]; int x2 = p2[0];
    int y1 = p1[1]; int y2 = p2[1];
    // if the line is steep, draw line by y value
    if(abs(p1[0] - p2[0]) < abs(p1[1] - p2[1])) 
    {
        std::swap(x1, y1);
        std::swap(x2, y2);
        steep = true;
    }
    // make it left-to-right
    if(x1 > x2)
    {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }

    int dx = x2 -x1;
    int dy = y2 - y1;
    int k = 2 * std::abs(dy); // slope
    int y = y1;
    int error = 0;
    // draw
    for(int x = x1; x< x2; x++)
    {
        if(steep)
            image.set(y, x, color);
        else
            image.set(x, y, color);

        error += k;
        if(error > dx)
        {
            y += y1 < y2? 1 : -1;
            error -= 2 * dx;
        }
    }
}