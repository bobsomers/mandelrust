#include <utility>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

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
    int iterations;
    Window window;
    std::vector<SampleOffset> sampleOffsets;
    std::vector<float> sampleWeights;
    float sampleWeightSum;
    int width;
    int height;
    int tileWidth;
    int tileHeight;
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

int
mandel(float cReal, float cImag, int iterations)
{
    float zReal = cReal;
    float zImag = cImag;

    int i = 0;
    for (; i < iterations; ++i) {
        float zRealSquared = zReal * zReal;
        float zImagSquared = zImag * zImag;

        if (zRealSquared + zImagSquared > 4.0f) break;

        float newReal = zRealSquared - zImagSquared;
        float newImag = 2.0f * zReal * zImag;
        
        zReal = cReal + newReal;
        zImag = cImag + newImag;
    }

    return i;
}

Rgb
shade(int val, int max)
{
    const float v = (val < max - 1) ? float(val) / max : 0.0f;
    return {v, v, v};
}

void
pixel(int x, int y, const Options& opts, std::vector<Rgb>& buf)
{

}

void
tile(int i, int j, const Options& opts, std::vector<Rgb>& buf)
{
    const int tileOffsetX = i * opts.tileWidth;
    const int tileOffsetY = j * opts.tileHeight;

    for (int ty = 0; ty < opts.tileHeight; ++ty) {
        const int y = tileOffsetY + ty;
        for (int tx = 0; tx < opts.tileWidth; ++tx) {
            const int x = tileOffsetX + tx;
            if (x >= opts.width || y >= opts.height) continue;
            pixel(x, y, opts, buf);
        }
    }
}

void
mandelbrot(const Options& opts, std::vector<Rgb>& buf)
{
    const int widthTiles = (opts.width / opts.tileWidth) +
            ((opts.width % opts.tileWidth > 0) ? 1 : 0);
    const int heightTiles = (opts.height / opts.tileHeight) +
            ((opts.height % opts.tileHeight > 0) ? 1 : 0);

    for (int j = 0; j < heightTiles; ++j) {
        for (int i = 0; i < widthTiles; ++i) {
            tile(i, j, opts, buf);
        }
    }
}

void
write(const Options& opts, const std::vector<Rgb>& buf)
{
    const float oneOverGamma = 1.0f / 2.2f;

    std::ofstream ppmFile("mandel.ppm", std::ios_base::out | std::ios_base::trunc);
    ppmFile << "P3 " << opts.width << " " << opts.height << " 255\n";
    for (int y = 0; y < opts.height; ++y) {
        for (int x = 0; x < opts.width; ++x) {
            const std::size_t index = y * opts.width + x;
            const Rgb& rgb = buf[index];

            // Gamma correction and 8-bit quantization.
            auto r8 = std::uint8_t(std::pow(rgb.r, oneOverGamma) * 255.0f);
            auto g8 = std::uint8_t(std::pow(rgb.g, oneOverGamma) * 255.0f);
            auto b8 = std::uint8_t(std::pow(rgb.b, oneOverGamma) * 255.0f);

            ppmFile << r8 << " " << g8 << " " << b8 << " ";
        }
        ppmFile << "\n";
    }
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
    //writeSamplingData();

    int width = 640;
    int height = 480;
    int tileWidth = 8;
    int tileHeight = 8;
    int samples = 64;
    int iterations = 256;
    float filterSize = 2.0f;
    Window window = {-2.0f, 1.0f, -1.0f, 1.0f};

    // Precompute subpixel sample offsets and weights.
    std::vector<SampleOffset> sampleOffsets(samples);
    std::vector<float> sampleWeights(samples);
    float sampleWeightSum = 0.0f;
    for (int i = 0; i < samples; ++i) {
        auto offset = halton23(i);
        offset.x = (offset.x - 0.5f) * filterSize;
        offset.y = (offset.y - 0.5f) * filterSize;

        const float weight = mitchellWeight(offset, filterSize / 2.0f);

        sampleOffsets[i] = offset;
        sampleWeights[i] = weight;
        sampleWeightSum += weight;
    }

    // Allocate image buffer.
    std::vector<Rgb> buf(width * height, {0.0f, 0.0f, 0.0f});

    Options opts;
    opts.iterations = iterations;
    opts.window = window;
    opts.sampleOffsets = std::move(sampleOffsets);
    opts.sampleWeights = std::move(sampleWeights);
    opts.sampleWeightSum = sampleWeightSum;
    opts.width = width;
    opts.height = height;
    opts.tileWidth = tileWidth;
    opts.tileHeight = tileHeight;

    mandelbrot(opts, buf);
}
