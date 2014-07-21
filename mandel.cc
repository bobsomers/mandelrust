#include <utility>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
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
    float x;
    float y;
    float width;
    float height;
};

struct Options
{
    int iterations;
    Window window;
    std::vector<SampleOffset> sampleOffsets;
    std::vector<float> sampleWeights;
    float oneOverSampleWeightSum;
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
pixel(int px, int py, const Options& opts, std::vector<Rgb>& buf)
{
    const float centerX = px + 0.5f;
    const float centerY = py + 0.5f;

    Rgb accum = {0.0f, 0.0f, 0.0f};

    std::size_t numSamples = opts.sampleOffsets.size();
    for (std::size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
        auto offset = opts.sampleOffsets[sampleIndex];
        
        // Map sample to window space.
        float x = (centerX + offset.x) / opts.width * opts.window.width + opts.window.x;
        float y = (centerY + offset.y) / opts.height * opts.window.height + opts.window.y;

        // Compute Mandelbrot set under the sample and shade it.
        int val = mandel(x, y, opts.iterations);
        Rgb rgb = shade(val, opts.iterations);

        // Accumulate a weighted sum of the shaded samples using the
        // precomputed sample weights from a Mitchell-Netravali filter.
        float weight = opts.sampleWeights[sampleIndex];
        accum.r += rgb.r * weight;
        accum.g += rgb.g * weight;
        accum.b += rgb.b * weight;
    }

    // Clamp low end to zero. Protects against cases where we only hit the
    // Mandelbrot with a single sample in the filter's negative lobe.
    accum.r = std::max(accum.r, 0.0f);
    accum.g = std::max(accum.g, 0.0f);
    accum.b = std::max(accum.b, 0.0f);

    // Compute weighted average from weighted sum.
    const float oneOverSampleWeightSum = opts.oneOverSampleWeightSum;
    Rgb weightedAverage = {
        accum.r * oneOverSampleWeightSum,
        accum.g * oneOverSampleWeightSum,
        accum.b * oneOverSampleWeightSum
    };

    // Write the weighted average into the buffer.
    const std::size_t index = py * opts.width + px;
    buf[index] = weightedAverage;
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
    const int numTiles = widthTiles * heightTiles;

#pragma omp parallel for
    for (int tileIndex = 0; tileIndex < numTiles; ++tileIndex) {
        int i = tileIndex % widthTiles;
        int j = tileIndex / widthTiles;
        tile(i, j, opts, buf);
    }
}

void
writePPM(const Options& opts, const std::vector<Rgb>& buf)
{
    const float oneOverGamma = 1.0f / 2.2f;

    std::ofstream ppmFile("mandel.ppm", std::ios_base::out | std::ios_base::trunc);
    ppmFile << "P3 " << opts.width << " " << opts.height << " 255\n";
    for (int y = 0; y < opts.height; ++y) {
        for (int x = 0; x < opts.width; ++x) {
            const std::size_t index = y * opts.width + x;
            const Rgb& rgb = buf[index];

            // Gamma correction and 8-bit quantization.
            auto r8 = int(std::pow(rgb.r, oneOverGamma) * 255.0f);
            auto g8 = int(std::pow(rgb.g, oneOverGamma) * 255.0f);
            auto b8 = int(std::pow(rgb.b, oneOverGamma) * 255.0f);

            ppmFile << std::to_string(r8) << " " <<
                       std::to_string(g8) << " " <<
                       std::to_string(b8) << " ";
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
    Window window = {-2.0f, -1.0f, 3.0f, 2.0f};

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
    opts.oneOverSampleWeightSum = 1.0f / sampleWeightSum;
    opts.width = width;
    opts.height = height;
    opts.tileWidth = tileWidth;
    opts.tileHeight = tileHeight;

    // Compute image and write it out;
    mandelbrot(opts, buf);
    writePPM(opts, buf);
}
