#include "chunk.h"
#include <iostream>
#include <ostream>
#include <QFile>
#include <string>

DrawableChunk::DrawableChunk(OpenGLContext *mp_context)
    : Drawable(mp_context), ptrsValid(false), vbo(nullptr), idx(nullptr), count(nullptr) {

}

void DrawableChunk::createVBOdata() {
    if (!ptrsValid) {
        throw std::out_of_range("attempting to draw chunk with possibly invalid pointers");
    }

    // At this point, VBO contains data of the entire chunk
    m_count = idx->size();

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx->size() * sizeof(GLuint), idx->data(), GL_STATIC_DRAW);

    generateInterleaved();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufInterleaved);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, vbo->size() * sizeof(glm::vec4), vbo->data(), GL_STATIC_DRAW);

    ptrsValid = false;
}

void DrawableChunk::create(std::vector<glm::vec4>& vbo, std::vector<GLuint>& idx)
{
    //    if (!ptrsValid) {
    //        throw std::out_of_range("attempting to draw chunk with possibly invalid pointers");
    //    }

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    generateInterleaved();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufInterleaved);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, vbo.size() * sizeof(glm::vec4), vbo.data(), GL_STATIC_DRAW);

    ptrsValid = false;
}

Chunk::Chunk(OpenGLContext *mp_context, int x, int z, int64_t key) : m_blocks(), minX(x), minZ(z), key(key), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}},
    cData(this), generated(false), trees(), opaque(mp_context), transparent(mp_context)
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}

const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

Direction vectorToDirection (glm::vec3 v) {
    if (v.x == 1) {
        return XPOS;
    }
    if (v.x == -1) {
        return XNEG;
    }
    if (v.y == 1) {
        return YPOS;
    }
    if (v.y == -1) {
        return YNEG;
    }
    if (v.z == 1) {
        return ZPOS;
    }
    return ZNEG;
}

// Array containing data for adjacent block faces for a block
// in the form {direction, [pos_array], [nor_array]} for each neighbor
const static std::array<BlockFace, 6> neighboring_faces {
    {
        {
            glm::vec3(-1, 0, 0),
            {glm::vec4(0, 0, 0, 1), glm::vec4(0, 0, 1, 1), glm::vec4(0, 1, 1, 1), glm::vec4(0, 1, 0, 1)},
            {glm::vec4(-1, 0, 0, 0), glm::vec4(-1, 0, 0, 0), glm::vec4(-1, 0, 0, 0), glm::vec4(-1, 0, 0, 0)}
        },

        {
            glm::vec3(1, 0, 0),
            {glm::vec4(1, 0, 1, 1), glm::vec4(1, 0, 0, 1), glm::vec4(1, 1, 0, 1), glm::vec4(1, 1, 1, 1)},
            {glm::vec4(1, 0, 0, 0), glm::vec4(1, 0, 0, 0), glm::vec4(1, 0, 0, 0), glm::vec4(1, 0, 0, 0)}
        },

        {
            glm::vec3(0, -1, 0),
            {glm::vec4(1, 0, 1, 1), glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 0, 1), glm::vec4(1, 0, 0, 1)},
            {glm::vec4(0, -1, 0, 0), glm::vec4(0, -1, 0, 0), glm::vec4(0, -1, 0, 0), glm::vec4(0, -1, 0, 0)}
        },

        {
            glm::vec3(0, 1, 0),
            {glm::vec4(1, 1, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 1, 1), glm::vec4(1, 1, 1, 1)},
            {glm::vec4(0, 1, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 1, 0, 0)}
        },

        {
            glm::vec3(0, 0, -1),
            {glm::vec4(1, 0, 0, 1), glm::vec4(0, 0, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(1, 1, 0, 1)},
            {glm::vec4(0, 0, -1, 0), glm::vec4(0, 0, -1, 0), glm::vec4(0, 0, -1, 0), glm::vec4(0, 0, -1, 0)}
        },

        {
            glm::vec3(0, 0, 1),
            {glm::vec4(0, 0, 1, 1), glm::vec4(1, 0, 1, 1), glm::vec4(1, 1, 1, 1), glm::vec4(0, 1, 1, 1)},
            {glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 1, 0)}
        }
    }
};

const static std::array<glm::vec2, 4> uv_offsets {
    glm::vec2(0,0), glm::vec2(1,0), glm::vec2(1,1), glm::vec2(0,1)
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

void Chunk::makeDrawableVBOs(std::vector<glm::vec4>& vboOpaque, std::vector<GLuint> &idxOpaque,
                             std::vector<glm::vec4>& vboTransparent, std::vector<GLuint> &idxTransparent)
{
    unsigned int v_countOpaque = -1; // A counter to keep track of how many vertices actually exist. It is the index of the last vertex.
    unsigned int v_countTransparent = -1;

    for (int z = 0; z < 16; z++)
    {
        for (int y = 0; y < 256; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                BlockType type = getBlockAt(x,y,z);
                // We only want to draw non-empty blocks
                if (type != EMPTY)
                {

                    BlockInfo info = block_info_map.at(type);

                    std::vector<glm::vec4>& vbo = (info.transparent) ? vboTransparent : vboOpaque;
                    std::vector<GLuint>& idx = (info.transparent) ? idxTransparent : idxOpaque;
                    unsigned int& v_count = (info.transparent) ? v_countTransparent : v_countOpaque;

                    // Iterate over the neighbors of this block
                    for (auto &n : neighboring_faces)
                    {
                        BlockType neighbor;
                        getNeighbor(x, y, z, n, neighbor);
                        // Only need to draw if neighbor is transparent
                        if (block_info_map.at(neighbor).transparent && neighbor != type)
                        {
                            // First position, then normal, then UV
                            for (int i = 0; i < 4; i++)
                            {
                                // pos nor uv animated

                                vbo.push_back(glm::vec4(x + minX, y, z + minZ, 0) + n.pos[i]);
                                glm::vec4 nor = n.nor[i];
                                vbo.push_back(nor);
                                glm::vec2 uv = info.uv_map.at(vectorToDirection(n.direction));
                                uv += uv_offsets[i];
                                uv /= 16.0f;
                                vbo.push_back(glm::vec4(uv,info.animated,-1));
                                v_count++;
                            }

                            // Triangulate the index buffer
                            for (int i = v_count - 2; i < v_count; i++)
                            {
                                idx.push_back(v_count-3);
                                idx.push_back(i);
                                idx.push_back(i+1);
                            }
                        }
                    }
                }
            }
        }
    }

    this->opaque.m_count = idxOpaque.size();
    this->transparent.m_count = idxTransparent.size();
}

void Chunk::resetVBOData() {
    destroyVBOData();

    std::vector<glm::vec4> vboOpaque;
    std::vector<GLuint> idxOpaque;
    std::vector<glm::vec4> vboTransparent;
    std::vector<GLuint> idxTransparent;
    makeDrawableVBOs(vboOpaque, idxOpaque, vboTransparent, idxTransparent);
    create(vboOpaque,idxOpaque,vboTransparent,idxTransparent);

}

// Helper function for obtaining the neighbor at position (x, y, z) in local chunk space
void Chunk::getNeighbor(int x, int y, int z, const BlockFace &n, BlockType &neighbor)
{
    if (x == 0 && n.direction.x == -1)
    {
        neighbor = (m_neighbors[XNEG] != nullptr) ? m_neighbors[XNEG]->getBlockAt(15, y, z) : EMPTY;
    }
    else if (x == 15 && n.direction.x == 1)
    {
        neighbor = (m_neighbors[XPOS] != nullptr) ? m_neighbors[XPOS]->getBlockAt(0, y, z) : EMPTY;
    }
    else if (z == 0 && n.direction.z == -1)
    {
        neighbor = (m_neighbors[ZNEG] != nullptr) ? m_neighbors[ZNEG]->getBlockAt(x, y, 15) : EMPTY;
    }
    else if (z == 15 && n.direction.z == 1)
    {
        neighbor = (m_neighbors[ZPOS] != nullptr) ? m_neighbors[ZPOS]->getBlockAt(x, y, 0) : EMPTY;
    }
    else if ((y == 0 && n.direction.y == -1) || (y == 255 && n.direction.y == 1))
    {
        neighbor = EMPTY;
    }
    else
    {
        neighbor = getBlockAt((int)(x + n.direction.x), (int)(y + n.direction.y), (int)(z + n.direction.z));
    }
}

glm::ivec2 Chunk::getMinPos() {
    return glm::ivec2(this->minX, this->minZ);
}

BlockType Chunk::getBlockByBiome(float height, bool onTop, BiomeType b, bool vFlip) {
    if (height <= 128) return BlockType::STONE;
    if (b == GRASSLANDS) {
        return onTop ? BlockType::GRASS : BlockType::DIRT;
    } else if (b == MOUNTAINS) {
        //switch to snow
        return height > 200 && onTop ? BlockType::SNOW : BlockType::STONE;
    } else if (b == VOLCANO) {
        return vFlip ? BlockType::LAVA : BlockType::OBSIDIAN;
    }
    return BlockType::EMPTY;
}

void Chunk::setChunkGenHeights() {
    glm::vec2 pos = this->getMinPos();
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            std::tuple<int, BiomeType, bool> biomeInfo = TerrainGen::getHeight(glm::vec2(pos.x + x, pos.y + z));
            int height = std::get<0>(biomeInfo);
            BiomeType b = std::get<1>(biomeInfo);
            bool vFlip = std::get<2>(biomeInfo);

            if (vFlip && b == VOLCANO) {
                height = 130;
            }

            glm::ivec2 tempPosVec2Tree = glm::ivec2(x, z);
            glm::ivec4 tempPosVec4Tree = glm::ivec4(pos.x + x, height, pos.y + z, 1);

            if (TerrainGen::WorleyNoise3(pos.x + x, pos.y + z) > 0.96 && b == GRASSLANDS) {
                if (treesMap.find(tempPosVec2Tree) == treesMap.end()) {
                    treesMap.insert({tempPosVec2Tree, tempPosVec4Tree});
                    trees.push_back(tempPosVec4Tree);
                }
            }

            //set blocks
            for (int y = 1; y <= height; y++) {
                BlockType type = getBlockByBiome(height, y == height, b, vFlip);
                this->setBlockAt(x, y, z, type);
                //cave range
                if (y < 150 && y > 0) {
                    float perlin3D = TerrainGen::perlinNoise3D(glm::vec3(pos.x + x, y, pos.y + z) / 25.f);

                    // "Floor" height of terrain is about 128
                    // Want Perlin value to increase closer to surface so caves are smaller
                    perlin3D += pow(glm::smoothstep(100.f, 130.f, (float)y), 2) * 0.3;

                    //cave limit of perlin val
                    if (perlin3D < 0.f) {
                        //25 -- above is empty, below is lava
                        this->setBlockAt(x, y, z, y > 25 ? BlockType::EMPTY : BlockType::LAVA);
                    }
                }
            }

            //make water pools from [128, 138)
            if (height >= 127 && this->getBlockAt(x, height, z) != BlockType::EMPTY && b != VOLCANO) {
                if (b != VOLCANO) {
                    for (int y = height + 1; y < 138; y++) {
                        if (this->getBlockAt(x, y, z) == BlockType::EMPTY) {
                            this->setBlockAt(x, y, z, BlockType::WATER);
                        }
                    }
                }
            }

//            //make lava pools from (130, 160)
//            if (height >= 110 && this->getBlockAt(x, height, z) != BlockType::EMPTY && b == VOLCANO) {
//                for (int y = height + 1; y < 160; y++) {
//                    if (this->getBlockAt(x, y, z) == BlockType::EMPTY) {
//                        this->setBlockAt(x, y, z, BlockType::LAVA);
//                    }
//                }
//            }
            this->setBlockAt(x, 0, z, BlockType::BEDROCK);
        }
    }

    generated.store(true);
}

bool Chunk::hasVBOData()
{
    return opaque.elemCount() != -1 && transparent.elemCount() != -1;
}

void Chunk::create(std::vector<glm::vec4>& vboDataOpaque, std::vector<GLuint>& idxDataOpaque,
                   std::vector<glm::vec4>& vboDataTransparent, std::vector<GLuint>& idxDataTransparent)
{
    this->opaque.create(vboDataOpaque, idxDataOpaque);
    this->transparent.create(vboDataTransparent, idxDataTransparent);
}

void Chunk::destroyVBOData()
{
    this->opaque.destroyVBOdata();
    this->transparent.destroyVBOdata();
}

std::vector<glm::ivec4>& Chunk::getTrees() {
    return this->trees;
}

void Chunk::clearTrees() {
    this->trees.clear();
}

std::unordered_map<Direction, Chunk*, EnumHash>& Chunk::getNeighbors() {
    return this->m_neighbors;
}

int64_t Chunk::getKey() {
    return key;
}

const static std::unordered_map<BlockType,unsigned char, EnumHash> block_to_char{
                                                                         {EMPTY,0x0},
    {GRASS,0x1},
    {DIRT,0x2},
    {STONE,0x3},
    {WATER,0x4},
    {SNOW,0x5},
    {LAVA,0x6},
    {BEDROCK,0x7},
    {GLASS,0x8},
    {OAK_LOG, 0x9},
    {OAK_LEAVES,0xA},
    {OBSIDIAN,0xB},
};
const static std::unordered_map<unsigned char, BlockType> char_to_block{
                                                                        {0x0,EMPTY},
    {0x1,GRASS},
    {0x2,DIRT},
    {0x3,STONE},
    {0x4,WATER},
    {0x5,SNOW},
    {0x6,LAVA},
    {0x7,BEDROCK},
    {0x8,GLASS},
    {0x9,OAK_LOG},
    {0xA,OAK_LEAVES},
    {0xB,OBSIDIAN},
};

void Chunk::load(QString savename) {
    QString keyStr = std::to_string(key).c_str();
    QFile file("../saves/"+savename+"/"+keyStr+".chunk");
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    long i = 0;
    while (!in.atEnd()) {
        unsigned char byte;
        in >> byte;
        m_blocks[i] = char_to_block.at(byte);
        i++;
    }

    file.close();

    generated.store(true);
}

void Chunk::save(QString savename) {
    QString keyStr = std::to_string(key).c_str();
    QFile file("../saves/"+savename+"/"+keyStr+".chunk");
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    for (BlockType block : m_blocks) {
        unsigned char byte = block_to_char.at(block);
        out << byte;
    }

    file.close();
}

bool Chunk::canCreate() {
    if (!generated.load() || hasVBOData()) return false;

    for (auto n : m_neighbors) {
        if (n.second == nullptr || !n.second->generated.load()) {
            return false;
        }
    }

    return true;
}
