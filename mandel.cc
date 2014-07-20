#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

struct Tile
{
    int i;
    int j;
};

struct Pixel
{
    int x;
    int y;
};

struct SampleOffset
{
    float x;
    float y;
};

struct Rgb
{
    float r;
    float g;
    float b;
};

struct Window
{
    float x0;
    float x1;
    float y0;
    float y1;
};

struct Options
{
    int width;
    int height;
    int iterations;
    Window window;
    std::vector<SampleOffset> sampleOffsets;
    std::vector<float> sampleWeights;
    float sampleWeightSum;
};

inline float
mitchell(float x, float b, float c)
{
    x = std::abs(2.0f * x);

    if (x < 1.0f) {
        return (1.0f / 6.0f) * (
                (12.0f - 9.0f*b - 6.0f*c) * x*x*x +
                (-18.0f + 12.0f*b + 6.0f*c) * x*x +
                (6.0f - 2.0f*b)
               );
    } else {
        return (1.0f / 6.0f) * (
                (-b - 6.0f*c) * x*x*x +
                (6.0f*b + 30.0f*c) * x*x +
                (-12.0f*b - 48.0f*c) * x +
                (8.0*b + 24.0*c)
               );
    }
}

inline float
mitchellWeight(SampleOffset offset, float size)
{
    const float oneOverSize = 1.0f / size;
    const float oneThird = 1.0f / 3.0f;

    const float mitchellX = mitchell(offset.x * oneOverSize, oneThird, oneThird);
    const float mitchellY = mitchell(offset.y * oneOverSize, oneThird, oneThird);

    return mitchellX * mitchellY;
}

inline float
halton(int index, int base)
{
    float result = 0.0f;
    float f = 1.0f / base;
    int i = index;

    while (i > 0) {
        result = result + f * (i % base);
        i = int(std::floor(float(i) / base));
        f = f / base;
    }

    return result;
}

inline SampleOffset
halton23(int index)
{
    return {halton(index, 2), halton(index, 3)};
}

void writeSamplingData()
{
    const float size = 2.0f;

    std::ofstream h23File("halton23.dat", std::ios_base::out | std::ios_base::trunc);
    h23File << "# X Y\n";
    for (int i = 0; i < 1024; ++i) {
        SampleOffset offset = halton23(i);
        offset.x = (offset.x - 0.5f) * size;
        offset.y = (offset.y - 0.5f) * size;
        h23File << offset.x << " " << offset.y << "\n";
    }

    std::ofstream m1dFile("mitchell_1d.dat", std::ios_base::out | std::ios_base::trunc);
    m1dFile << "# X Y\n";
    const float oneThird = 1.0f / 3.0f;
    for (float x = -2.0f; x <= 2.0f; x += 0.01f) {
        float weight = mitchell(x / 2.0f, oneThird, oneThird);
        m1dFile << x << " " << weight << "\n";
    }

    std::ofstream m2dFile("mitchell_2d.dat", std::ios_base::out | std::ios_base::trunc);
    m2dFile << "# X Y Z\n";
    for (int i = 0; i < 1024; ++i) {
        SampleOffset offset = halton23(i);
        offset.x = (offset.x - 0.5f) * size;
        offset.y = (offset.y - 0.5f) * size;
        float weight = mitchellWeight(offset, size / 2.0f);
        m2dFile << offset.x << " " << offset.y << " " << weight << "\n";
    }
}

int main() {
    writeSamplingData();
}
