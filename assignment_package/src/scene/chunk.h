#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "drawable.h"
#include <array>
#include <unordered_map>
#include <cstddef>
#include <atomic>

#include "terraingen.h"


//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, SNOW, LAVA, BEDROCK, GLASS, OAK_LOG, OAK_LEAVES, OBSIDIAN, BONE
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

struct EnumPairHash {
public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &x) const
    {
        return EnumHash()(x.first) ^ EnumHash()(x.second);
    }
};

struct BlockFace
{
    glm::vec3 direction;
    std::array<glm::vec4, 4> pos;
    std::array<glm::vec4, 4> nor;
};

struct BlockInfo {
    std::unordered_map<Direction,glm::vec2,EnumHash> uv_map;
    bool transparent;
    float animated;
};

// block info is uv texture map, transparent, animated (1 or 0)
const static std::unordered_map<BlockType, BlockInfo, EnumHash> block_info_map = {
    {EMPTY, {{{XPOS,glm::vec2(13,2)},
                 {XNEG,glm::vec2(13,2)},
                 {YPOS,glm::vec2(13,2)},
                 {YNEG,glm::vec2(13,2)},
                 {ZPOS,glm::vec2(13,2)},
                 {ZNEG,glm::vec2(13,2)}},
                true, 0.0f
            }},
    {GRASS, {{{XPOS,glm::vec2(3,15)},
                 {XNEG,glm::vec2(3,15)},
                 {YPOS,glm::vec2(8,13)},
                 {YNEG,glm::vec2(2,15)},
                 {ZPOS,glm::vec2(3,15)},
                 {ZNEG,glm::vec2(3,15)}},
                false, 0.0f
            }},
    {DIRT, {{{XPOS,glm::vec2(2,15)},
                {XNEG,glm::vec2(2,15)},
                {YPOS,glm::vec2(2,15)},
                {YNEG,glm::vec2(2,15)},
                {ZPOS,glm::vec2(2,15)},
                {ZNEG,glm::vec2(2,15)}},
               false, 0.0f
           }},
    {STONE, {{{XPOS,glm::vec2(1,15)},
                 {XNEG,glm::vec2(1,15)},
                 {YPOS,glm::vec2(1,15)},
                 {YNEG,glm::vec2(1,15)},
                 {ZPOS,glm::vec2(1,15)},
                 {ZNEG,glm::vec2(1,15)}},
                false, 0.0f
            }},
    {WATER, {{{XPOS,glm::vec2(14,3)},
                 {XNEG,glm::vec2(14,3)},
                 {YPOS,glm::vec2(14,3)},
                 {YNEG,glm::vec2(14,3)},
                 {ZPOS,glm::vec2(14,3)},
                 {ZNEG,glm::vec2(14,3)}},
                true, 1.0f
            }},
    {SNOW, {{{XPOS,glm::vec2(2,11)},
                {XNEG,glm::vec2(2,11)},
                {YPOS,glm::vec2(2,11)},
                {YNEG,glm::vec2(2,11)},
                {ZPOS,glm::vec2(2,11)},
                {ZNEG,glm::vec2(2,11)}},
               false, 0.0f
           }},
    {LAVA, {{{XPOS,glm::vec2(13,1)},
                {XNEG,glm::vec2(13,1)},
                {YPOS,glm::vec2(13,1)},
                {YNEG,glm::vec2(13,1)},
                {ZPOS,glm::vec2(13,1)},
                {ZNEG,glm::vec2(13,1)}},
               false, 1.0f
           }},
    {BEDROCK, {{{XPOS,glm::vec2(1,14)},
                   {XNEG,glm::vec2(1,14)},
                   {YPOS,glm::vec2(1,14)},
                   {YNEG,glm::vec2(1,14)},
                   {ZPOS,glm::vec2(1,14)},
                   {ZNEG,glm::vec2(1,14)}},
                  false, 0.0f
              }},
    {GLASS, {{{XPOS,glm::vec2(1,12)},
                 {XNEG,glm::vec2(1,12)},
                 {YPOS,glm::vec2(1,12)},
                 {YNEG,glm::vec2(1,12)},
                 {ZPOS,glm::vec2(1,12)},
                 {ZNEG,glm::vec2(1,12)}},
                true, 0.0f
            }},
    {OAK_LOG, {{{XPOS,glm::vec2(4, 14)},
                {XNEG, glm::vec2(4, 14)},
                {YPOS, glm::vec2(5, 14)},
                {YNEG, glm::vec2(5, 14)},
                {ZPOS, glm::vec2(4, 14)},
                {ZNEG, glm::vec2(4, 14)}},
               false, 0.0f}},
    {OAK_LEAVES, {{{XPOS,glm::vec2(4, 12)},
                   {XNEG, glm::vec2(4, 12)},
                   {YPOS, glm::vec2(4, 12)},
                   {YNEG, glm::vec2(4, 12)},
                   {ZPOS, glm::vec2(4, 12)},
                   {ZNEG, glm::vec2(4, 12)}},
                  true, 0.0f}},
    {OBSIDIAN, {{{XPOS,glm::vec2(7, 4)},
                 {XNEG, glm::vec2(7, 4)},
                 {YPOS, glm::vec2(7, 4)},
                 {YNEG, glm::vec2(7, 4)},
                 {ZPOS, glm::vec2(7, 4)},
                 {ZNEG, glm::vec2(7, 4)}},
                false, 0.0f}},
    {BONE, {{{XPOS,glm::vec2(15,4)}},false,0.0f}},
    };

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

class Chunk;

struct ChunkVBOData
{
    Chunk* chunk;
    std::vector<glm::vec4> vboDataOpaque, vboDataTransparent;
    std::vector<GLuint> idxDataOpaque, idxDataTransparent;

    ChunkVBOData(Chunk* c) :
        chunk(c), vboDataOpaque{}, vboDataTransparent{}, idxDataOpaque{}, idxDataTransparent{}
    {}
};

class DrawableChunk : public Drawable {
    friend class Chunk;
private:
    bool ptrsValid;
    std::vector<glm::vec4>* vbo;
    std::vector<GLuint>* idx;
    unsigned int* count;

    virtual void createVBOdata();

    // for creating chunks using data given by threads
    void create(std::vector<glm::vec4> &vbo, std::vector<GLuint> &idx);
public:
    DrawableChunk(OpenGLContext* mp_context);
};

struct single_hash {
    template <typename T>
    std::size_t operator () (const T& input) const {
        return std::hash<T>{}(input);
    }
};

struct hash_ivec2 {
    std::size_t operator()(const glm::ivec2& v) const {
        std::hash<int> hasher;
        size_t hashValue = 5381; // Initial value for djb2 algorithm

        // Combine hash values of x and y using djb2 algorithm
        hashValue = ((hashValue << 5) + hashValue) + hasher(v.x);
        hashValue = ((hashValue << 5) + hashValue) + hasher(v.y);

        return hashValue;
    }
};

// TODO have Chunk inherit from Drawable
class Chunk {
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    int minX, minZ;
    int64_t key;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    ChunkVBOData cData;

    std::unordered_map<glm::ivec2, glm::ivec4, hash_ivec2> treesMap;
    std::vector<glm::ivec4> trees;

    std::atomic_bool generated;

public:
    Chunk(OpenGLContext* mp_context);
    Chunk(OpenGLContext* mp_context, int x, int z, int64_t key);

    DrawableChunk opaque;
    DrawableChunk transparent;

    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);

    // VBO methods:
    // Populates the reference vectors for use in threading
    void makeDrawableVBOs(std::vector<glm::vec4>& vboOpaque, std::vector<GLuint>& idxOpaque,
                          std::vector<glm::vec4>& vboTransparent, std::vector<GLuint>& idxTransparent);

    void unravelVBO(std::vector<glm::vec4> &vbo, std::vector<GLuint> &idx,
                    std::vector<glm::vec4> &pos, std::vector<glm::vec4> &col, std::vector<glm::vec4> &nor, std::vector<glm::vec4>& uv);


    //void createVBOData();
//    void unravelVBO(std::vector<glm::vec4> &vbo, std::vector<GLuint> &idx,
//                    std::vector<glm::vec4> &pos, std::vector<glm::vec4> &col, std::vector<glm::vec4> &nor, std::vector<glm::vec4>& uv);
    void destroyVBOData();
    void resetVBOData();

    // Helper methods:
    void getNeighbor(int x, int y, int z, const BlockFace &n, BlockType &neighbor);

    glm::ivec2 getMinPos();

    BlockType getBlockByBiome(float height, bool onTop, BiomeType b, bool vFlip);
    void setChunkGenHeights();
    bool hasVBOData();
    // Sends data for opaque and transparent to GPU
    void create(std::vector<glm::vec4>& vboDataOpaque, std::vector<GLuint>& idxDataOpaque,
                std::vector<glm::vec4>& vboDataTransparent, std::vector<GLuint>& idxDataTransparent);

    std::vector<glm::ivec4>& getTrees();
    void clearTrees();
    std::unordered_map<Direction, Chunk*, EnumHash>& getNeighbors();

    int64_t getKey();
    void load(QString savename);
    void save(QString savename);

    bool canCreate();
};

