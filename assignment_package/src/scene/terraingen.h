#ifndef TERRAINGEN_H
#define TERRAINGEN_H

#include "la.h"
#include <cmath>
#include <random>
#include <stack>

enum BiomeType {
    GRASSLANDS,
    MOUNTAINS,
    VOLCANO
};

class TerrainGen {
private:
    static glm::vec2 random2(glm::vec2 p);
    static float noise2D(glm::vec2 p);
    static float interpNoise2D(glm::vec2 xy);
    static float fbm(glm::vec2 xy);

    static float WorleyNoise(glm::vec2 uv);
    static std::pair<float, bool> WorleyNoise2 (glm::vec2 uv);

    static float surflet(glm::vec2 P, glm::vec2 gridPoint);
    static float PerlinNoise(glm::vec2 uv);

    static glm::vec3 random3(glm::vec3 xyz);
    static float surflet3D(glm::vec3 p, glm::vec3 gridPoint);

    static float getMountainH(glm::vec2 xz);
    static float getGrasslandsH(glm::vec2 xz);
    static std::pair<float, bool> getVolcanoH(glm::vec2 xz);

    static float BiomeBlender(glm::vec2 xz);

    static float seed;
public:
    TerrainGen();
    static std::tuple<int, BiomeType, bool> getHeight(glm::vec2 xz);
    static float perlinNoise3D(glm::vec3 p);
    static float WorleyNoise3(float x, float y);
    static void setSeed(float seed);
};

#endif // TERRAINGEN_H
