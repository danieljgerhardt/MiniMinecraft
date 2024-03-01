#include "terraingen.h"
#include <iostream>

TerrainGen::TerrainGen() {}

float TerrainGen::seed = 0;

glm::vec2 TerrainGen::random2(glm::vec2 p) {
    return glm::fract(glm::cos(
                          glm::vec2(glm::dot(p, glm::vec2(57.2, 23.2)), glm::dot(p, glm::vec2(152.4, 777777.44))) + glm::vec2(seed,seed))
                      * glm::vec2(998877.654321));
}

float TerrainGen::noise2D(glm::vec2 p) {
    return random2(p).x;
}

float TerrainGen::interpNoise2D(glm::vec2 xy) {
    int intX = int(floor(xy.x));
    float fractX = glm::fract(xy.x);
    int intY = int(floor(xy.y));
    float fractY = glm::fract(xy.y);

    float v1 = noise2D(glm::vec2(intX, intY));
    float v2 = noise2D(glm::vec2(intX + 1, intY));
    float v3 = noise2D(glm::vec2(intX, intY + 1));
    float v4 = noise2D(glm::vec2(intX + 1, intY + 1));

    float i1 = glm::mix(v1, v2, fractX);
    float i2 = glm::mix(v3, v4, fractX);
    return glm::mix(i1, i2, fractY);
}


float TerrainGen::fbm(glm::vec2 xy) {
    float total = 0;
    float persistence = 0.7f;
    int octaves = 6;
    float freq = 4.f;
    float amp = 0.5f;
    for(int i = 1; i <= octaves; i++) {
        total += interpNoise2D(glm::vec2(xy.x * freq,
                                         xy.y * freq)) * amp;

        freq *= 2.f;
        amp *= persistence;
    }
    return total;
}

float TerrainGen::WorleyNoise(glm::vec2 uv) {
    uv *= 5.0; // Now the space is 10x10 instead of 1x1. Change this to any number you want.
    glm::vec2 uvInt = glm::floor(uv);
    glm::vec2 uvFract = glm::fract(uv);
    float minDist1 = 1.0; // Minimum distance initialized to max.
    float minDist2 = 1.0; // Second minimum distance initialized to max.
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            glm::vec2 neighbor = glm::vec2(float(x), float(y)); // Direction in which neighbor cell lies
            glm::vec2 point = random2(uvInt + neighbor); // Get the Voronoi centerpoint for the neighboring cell
            glm::vec2 diff = neighbor + point - uvFract; // Distance between fragment coord and neighbor’s Voronoi point
            float dist = glm::length(diff);
            if (dist < minDist1) {
                minDist2 = minDist1;
                minDist1 = dist;
            } else if (dist < minDist2) {
                minDist2 = dist;
            }
        }
    }
    return minDist2 - minDist1;
}

std::pair<float, bool> TerrainGen::WorleyNoise2(glm::vec2 uv) {
    uv *= 10.0; // Now the space is 10x10 instead of 1x1. Change this to any number you want.
    glm::vec2 uvInt = glm::floor(uv);
    glm::vec2 uvFract = glm::fract(uv);
    float minDist = 1.0; // Minimum distance initialized to max.
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            glm::vec2 neighbor = glm::vec2(float(x), float(y)); // Direction in which neighbor cell lies
            glm::vec2 point = random2(uvInt + neighbor); // Get the Voronoi centerpoint for the neighboring cell
            glm::vec2 diff = neighbor + point - uvFract; // Distance between fragment coord and neighbor’s Voronoi point
            float dist = glm::length(diff);
            if (dist < minDist) {
                minDist = dist;
            }
        }
    }
    //smaller than the if number = smaller volcano hole
    if (minDist < 0.15) {
        //higher mult = lower volcano hole
        //minDist = (1 - minDist) * 0.9;
        return {minDist, true};
    }
    return {minDist, false};
}

float TerrainGen::WorleyNoise3(float x, float y) {
    glm::vec2 uv = glm::vec2(x, y);
    uv *= 10.0; // Now the space is 10x10 instead of 1x1. Change this to any number you want.
    glm::vec2 uvInt = glm::floor(uv);
    glm::vec2 uvFract = glm::fract(uv);
    float minDist = 1.0;
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            glm::vec2 neighbor = glm::vec2(float(x), float(y)); // Direction in which neighbor cell lies
            glm::vec2 point = random2(uvInt + neighbor); // Get the Voronoi centerpoint for the neighboring cell
            glm::vec2 diff = neighbor + point - uvFract; // Distance between fragment coord and neighbor’s Voronoi point
            float dist = glm::length(diff);
            if (dist < minDist) {
                minDist = dist;
            }
        }
    }
    return minDist;
}

float TerrainGen::surflet(glm::vec2 P, glm::vec2 gridPoint) {
    // Compute falloff function by converting linear distance to a polynomial
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float tX = 1 - 6 * pow(distX, 5.f) + 15 * pow(distX, 4.f) - 10 * pow(distX, 3.f);
    float tY = 1 - 6 * pow(distY, 5.f) + 15 * pow(distY, 4.f) - 10 * pow(distY, 3.f);
    // Get the random vector for the grid point
    glm::vec2 gradient = 2.f * random2(gridPoint) - glm::vec2(1.f);
    // Get the vector from the grid point to P
    glm::vec2 diff = P - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = glm::dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY;
}


float TerrainGen::PerlinNoise(glm::vec2 uv) {
    uv *= 10.0;
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            surfletSum += surflet(uv, glm::floor(uv) + glm::vec2(dx, dy));
        }
    }
    return surfletSum;
}

glm::vec3 TerrainGen::random3(glm::vec3 xyz) {
    return glm::fract(glm::sin(
                          glm::vec3(
                              glm::dot(xyz, glm::vec3(12, 21412.2, 124.23)),
                              glm::dot(xyz, glm::vec3(123.1, 235.2, 124.6)),
                              glm::dot(xyz, glm::vec3(214.5, 83.2, 555))) + glm::vec3(seed,seed,seed)) * glm::vec3(43758.5453));
}

float TerrainGen::surflet3D(glm::vec3 p, glm::vec3 gridPoint) {
    // Compute the distance between p and the grid point along each axis, and warp it with a
    // quintic function so we can smooth our cells
    glm::vec3 t2 = glm::abs(p - gridPoint);
    glm::vec3 t = glm::vec3(1.f) - 6.f * glm::pow(t2, glm::vec3(5.f, 5.f, 5.f))
                  + 15.f * glm::pow(t2, glm::vec3(4.f, 4.f, 4.f))
                  - 10.f * glm::pow(t2, glm::vec3(3.f, 3.f, 3.f))
                  + glm::vec3(0.0001f);
    // Get the random vector for the grid point (assume we wrote a function random2
    // that returns a vec2 in the range [0, 1])
    glm::vec3 gradient = random3(gridPoint) * glm::vec3(2.f, 2.f, 2.f) - glm::vec3(1.f, 1.f, 1.f);
    // Get the vector from the grid point to P
    glm::vec3 diff = p - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = glm::dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * t.x * t.y * t.z;
}

float TerrainGen::perlinNoise3D(glm::vec3 p) {
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            for(int dz = 0; dz <= 1; ++dz) {
                surfletSum += surflet3D(p, glm::floor(p) + glm::vec3(dx, dy, dz));
            }
        }
    }
    return surfletSum;
}


float TerrainGen::getMountainH(glm::vec2 xz) {

    float mountainsH = 0.f;

    float freq = 2400;
    float amp = 0.55;
    for (int i = 0; i < 4; i++) {
        float h1 = WorleyNoise(xz / freq);
        h1 = h1 * 2 - 1;
        h1 = abs(h1);
        h1 = pow(h1, 1.2);
        mountainsH += h1 * amp;
        amp *= 0.5;
        freq *= 0.5;
    }
    mountainsH = floor(135 + mountainsH * 110);

    return mountainsH;
}

float TerrainGen::getGrasslandsH(glm::vec2 xz) {

    float grasslandsH = 0.f;

    float amp = 0.6;
    float freq = 630;

    for (int i = 0; i < 4; i++) {
        float perlin = (PerlinNoise(xz / (freq)) + 1.f) / 2.f;
        float h1 = perlin;
        grasslandsH += h1 * amp;
        amp *= 0.6; // Adjusted amplitude decay for a smoother transition
        freq *= 0.7; // Adjusted frequency decay for more variation
    }

    grasslandsH = floor(106 + grasslandsH * 60);

    return grasslandsH;
}

std::pair<float, bool> TerrainGen::getVolcanoH(glm::vec2 xz) {
    float volcanoH = 0.f;
    float freq = 2600.f;
    std::pair<float, bool> worleyRes = WorleyNoise2(xz / freq);
    float w = 1 - worleyRes.first;
    volcanoH += w;

    volcanoH = floor(50 + volcanoH * 206);

    return {volcanoH, worleyRes.second};
}

float TerrainGen::BiomeBlender(glm::vec2 xz) {
    return 0.5 * (5 * (PerlinNoise(xz / 4000.f)) + 1.f);
}

std::tuple<int, BiomeType, bool> TerrainGen::getHeight(glm::vec2 xz) {
    float g = getGrasslandsH(xz), m = getMountainH(xz);
    std::pair<float, bool> v = getVolcanoH(xz);
    float blend = glm::smoothstep(0.35f, 0.5f, BiomeBlender(xz));
    float height = glm::mix(g, m, blend);
    height = glm::min(height,255.f);

    BiomeType b = blend < 0.6f ? GRASSLANDS : MOUNTAINS;

    float blend2 = glm::smoothstep(170.f, 200.f, v.first);
    height = glm::mix(height, v.first, blend2);
    if (blend2 > 0.6) {
        b = VOLCANO;
    }
    return std::tuple<int, BiomeType, bool>(floor(height), b, v.second);
}

void TerrainGen::setSeed(float seed) {
    TerrainGen::seed = seed;
}
