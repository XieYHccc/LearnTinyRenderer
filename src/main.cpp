#include <vector>
#include <cmath>

#include "tgaimage.h"
#include "vector.h"
#include "renderer.h"

const TGAColor white{255, 255, 255, 255, 4};
const TGAColor red{ 0,   0,   255, 255, 4};
const TGAColor green{0, 255, 0, 4};
const int width = 600;
const int height =600;

int main(int argc, char** argv) {
        TGAImage image(100, 100, TGAImage::RGB);
        line(ivec2(13, 20), ivec2(80, 40), image, white);
        line(ivec2(20, 13), ivec2(40, 80), image, red);
        line(ivec2(80, 40), ivec2(13, 20), image, red);  
        image.write_tga_file("../build/output.tga");
        return 0;
}