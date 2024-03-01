#include "terrain.h"
#include <stack>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <mutex>
#include <QFile>

#define SDF_R 3.f

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context),
    chunks_needing_generated(), chunks_needing_generated_mutexes(),
    chunks_needing_created(), chunks_needing_created_mutexes(),
    created_chunks(), created_chunks_mutex(),
    threads(),
    terrain_timer(0.f), prev_pos(glm::vec3()), trees(),
    saved(), savedMutex(), updated(), savename(),
    chunks_needing_saving(), chunks_needing_saving_mutexes(), seed(), idle_threads(), terminate(false)
{
    for (int i = 0; i < NUM_CORES; i++)
    {
        threads.push_back(std::thread(multithread_work,
                                      std::ref(chunks_needing_generated[i]), std::ref(chunks_needing_generated_mutexes[i]),
                                      std::ref(chunks_needing_created[(i+1)%NUM_CORES]), std::ref(chunks_needing_created_mutexes[(i+1)%NUM_CORES]),
                                      std::ref(chunks_needing_saving[(i+2)%NUM_CORES]), std::ref(chunks_needing_saving_mutexes[(i+2)%NUM_CORES]),
                                      std::ref(this->created_chunks), std::ref(this->created_chunks_mutex),
                                      std::ref(this->saved), std::ref(this->savedMutex), std::ref(this->savename), std::ref(idle_threads[i]),std::ref(terminate)));
    }
}

Terrain::Terrain(OpenGLContext *context, QString savename)
    : m_chunks(), m_generatedTerrain(), mp_context(context),
    chunks_needing_generated(), chunks_needing_generated_mutexes(),
    chunks_needing_created(), chunks_needing_created_mutexes(),
    created_chunks(), created_chunks_mutex(),
    threads(),
    terrain_timer(0.f), prev_pos(glm::vec3()),
    saved(), savedMutex(), updated(), savename(savename),
    chunks_needing_saving(), chunks_needing_saving_mutexes(), seed(), idle_threads(), terminate(false)
{
    load(savename);

    for (int i = 0; i < NUM_CORES; i++)
    {
        threads.push_back(std::thread(multithread_work,
                                      std::ref(chunks_needing_generated[i]), std::ref(chunks_needing_generated_mutexes[i]),
                                      std::ref(chunks_needing_created[(i+1)%NUM_CORES]), std::ref(chunks_needing_created_mutexes[(i+1)%NUM_CORES]),
                                      std::ref(chunks_needing_saving[(i+2)%NUM_CORES]), std::ref(chunks_needing_saving_mutexes[(i+2)%NUM_CORES]),
                                      std::ref(this->created_chunks), std::ref(this->created_chunks_mutex),
                                      std::ref(this->saved), std::ref(this->savedMutex), std::ref(this->savename), std::ref(idle_threads[i]),std::ref(terminate)));
    }
}

Terrain::~Terrain() {
    for (int64_t key : m_generatedTerrain)
    {
        m_chunks[key]->destroyVBOData();
    }

    terminate.store(true);
    for (int i = 0; i < NUM_CORES; i++) {
        threads[i].join();
    }
}

// Array containing ivec2's representing the translation distance to each of the surrounding
// 8 neighbors of a chunk. Starting from lower-left and moving CCW if x is vertical axis and z is horizontal axis
const static std::array<glm::ivec2, 8> neighboring_chunks {
    glm::ivec2(-16, -16), glm::ivec2(-16, 0), glm::ivec2(-16, 16),
    glm::ivec2(0, 16), glm::ivec2(16, 16), glm::ivec2(16, 0),
    glm::ivec2(16, -16), glm::ivec2(0, -16)
};

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}

uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}

const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);

        updated.insert(c.get());
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(this->mp_context, x, z, toKey(x,z));
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = std::move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram) {
    for (bool opaque : {true, false}) {
        for (int x = minX; x < maxX; x += 16)
        {
            for (int z = minZ; z < maxZ; z += 16)
            {
                if (hasChunkAt(x, z))
                {
                    const uPtr<Chunk> &chunk = getChunkAt(x, z);
                    if (chunk->hasVBOData())
                    {
                        //shaderProgram->setModelMatrix(glm::translate(glm::mat4(), glm::vec3(x, 0, z)));
                        shaderProgram->drawInterleaved((opaque) ? chunk->opaque : chunk->transparent);
                    }
                }
            }
        }
    }
}

void Terrain::tick(const glm::vec3 &player_pos, float dt)
{
    terrain_timer += dt;
    if (terrain_timer < 1.f)
    {
        return;
    }
    tryExpansion(player_pos, this->prev_pos);

    // Send VBO data to the GPU in main thread
    created_chunks_mutex.lock();
    for (auto &d : created_chunks)
    {
        d.chunk->create(d.vboDataOpaque, d.idxDataOpaque,
                        d.vboDataTransparent, d.idxDataTransparent);
        //get the chunks trees
        std::vector<glm::ivec4> currChunkTrees = d.chunk->getTrees();
        for (auto& t : currChunkTrees) {
            this->createTree(t);
        }
        d.chunk->clearTrees();
    }
    created_chunks.clear();
    created_chunks_mutex.unlock();

    // set timer back to 0 and set prev_pos if we reach this point
    terrain_timer = 0;
    this->prev_pos = player_pos;
}

void Terrain::tryExpansion(const glm::vec3 &player_pos, const glm::vec3 &prev_pos)
{
    // Look at surrounding 5x5 area of terrain generation zones, and
    // generate the terrain if a zone does not exist

    // First compute the set of 5x5 terrain zones for player's current position
    // and the player's previous position
    std::unordered_set<int64_t> curr_rad, prev_rad;
    for (int x = -64 * CREATE_RADIUS; x <= 64 * CREATE_RADIUS; x += 64)
    {
        for (int z = -64 * CREATE_RADIUS; z <= 64 * CREATE_RADIUS; z += 64)
        {
            curr_rad.insert(toKey(64 * static_cast<int>(glm::floor((player_pos.x + x) / 64.f)), 64 * static_cast<int>(glm::floor((player_pos.z + z) / 64.f))));
            prev_rad.insert(toKey(64 * static_cast<int>(glm::floor((prev_pos.x + x) / 64.f)), 64 * static_cast<int>(glm::floor((prev_pos.z + z) / 64.f))));
        }
    }

    // Delete VBO data for old terrain, i.e., keys found in PREV and not CURR
    for (int64_t key : prev_rad)
    {
        if (curr_rad.find(key) == curr_rad.end())
        {
            deleteTerrain(key);
        }
    }

    // Generate new terrain if it does not exist
    // If it does exist, send to be processed for VBO data
    for (int64_t key : curr_rad)
    {
        glm::ivec2 coords = toCoords(key);
        int i = 0;
        if (m_generatedTerrain.find(key) == m_generatedTerrain.end())
        {
            // Send chunk data to threads to be processed for blocktype data
            for (int x = coords.x; x < coords.x + 64; x += 16)
            {
                for (int z = coords.y; z < coords.y + 64; z += 16)
                {
                    Chunk* c = instantiateChunkAt(x, z);
                    chunks_needing_generated_mutexes[i].lock();
                    chunks_needing_generated[i].push_back(c);
                    chunks_needing_generated_mutexes[i].unlock();
                    i = (i + 1) % NUM_CORES;
                }
            }
            m_generatedTerrain.insert(key);
        }
        else
        {
            // Send chunk data to threads to be processed for vbo data
            for (int x = coords.x; x < coords.x + 64; x += 16)
            {
                for (int z = coords.y; z < coords.y + 64; z += 16)
                {
                    Chunk* c = getChunkAt(x, z).get();
                    if (c && c->canCreate() /*&& prev_rad.find(key) == prev_rad.end()*/)
                    {
                        chunks_needing_created_mutexes[i].lock();
                        chunks_needing_created[i].push_back(c);
                        chunks_needing_created_mutexes[i].unlock();
                        i = (i + 1) % NUM_CORES;
                    }
                }
            }
        }
    }
}

void Terrain::deleteTerrain(int64_t &key)
{
    glm::ivec2 coords = toCoords(key);
    for (int x = coords.x; x < coords.x + 64; x += 16)
    {
        for (int z = coords.y; z < coords.y + 64; z += 16)
        {
            Chunk* c = getChunkAt(x, z).get();
            if (c->hasVBOData())
            {
                c->destroyVBOData();
            }
        }
    }
}

void Terrain::createTerrain() {}

void Terrain::createTree(glm::ivec4& pos) {
    uPtr<Tree> newTree = mkU<Tree>(pos, 1);
    this->buildTree(pos, newTree->getAxiom());
    this->trees.push_back(std::move(newTree));
}

void Terrain::growTrees(qint64 currTime) {
    for (uPtr<Tree>& t : trees) {
        if ((currTime + t->getOffset()) % 10000 == 0) {
            t->performIteration();
            this->buildTree(t->getPos(), t->getAxiom());
        }
    }
}

//p = the point to calculate
//a = first endpoint, b = second endpoint
//r = radius, 0.5 for block
float sdCapsule(glm::vec3 p, glm::vec3 a, glm::vec3 b, float r) {
    glm::vec3 pa = p - a, ba = b - a;
    float h = glm::clamp(glm::dot(pa,ba) / glm::dot(ba,ba), 0.0f, 1.0f);
    return glm::length(pa - ba * h) - r;
}

float opS(float d1, float d2) {
    return std::max(-d1, d2);
}

float sdHollowCapsule(glm::vec3 p, glm::vec3 a, glm::vec3 b, float innerR, float outerR) {
    float d1 = sdCapsule(p, a, b, innerR);
    float d2 = sdCapsule(p, a, b, outerR);
    return opS(d1, d2);
}

float sdSphere(glm::vec3 p, float s) {
    return glm::length(p) - s;
}

float sdCutSphere(glm::vec3 p, float r, float h) {
    float w = sqrt(r * r - h * h);
    glm::vec2 q = glm::vec2(glm::length(glm::vec2(p.x, p.z)), p.y);
    float s = std::max((h - r) * q.x * q.x + w * w * (h + r - 2.0f * q.y), h * q.x - w * q.y);
    return (s < 0.0f) ? glm::length(q) - r : (q.x < w) ? h - q.y : glm::length(q - glm::vec2(w, h));
}

std::vector<glm::ivec4> getBlocks(glm::vec3 startPos, glm::vec3 endPos) {
    std::vector<glm::ivec4> ret;

    for (int x = std::min(startPos.x, endPos.x); x <= std::max(startPos.x, endPos.x); ++x) {
        for (int y = std::min(startPos.y, endPos.y); y <= std::max(startPos.y, endPos.y); ++y) {
            for (int z = std::min(startPos.z, endPos.z); z <= std::max(startPos.z, endPos.z); ++z) {
                glm::vec3 temp = glm::vec3(x, y, z);
                if (sdCapsule(temp, startPos, endPos, SDF_R) < 0) {
                    ret.push_back(glm::ivec4(temp.x, temp.y, temp.z, 1));
                }
            }
        }
    }

    return ret;
}


void Terrain::buildTree(glm::ivec4& pos, std::vector<TreeSymbol>& axiom) {
    Turtle turtle = {glm::vec4(pos), 0, 0, false};
    std::stack<Turtle> turtleStack;
    float length = 2;
    //chunk -> {logs, leaves} map
    std::unordered_map<Chunk*, std::pair<std::vector<glm::ivec4>, std::vector<glm::ivec4>>, single_hash> buildMap;
    
    for (TreeSymbol& symbol : axiom) {
        glm::vec4& tempPos = turtle.pos;
        float& tempX = turtle.xAmt;
        float& tempZ = turtle.zAmt;
        switch (symbol.getSym()) {
        case T:
            for (auto& ref : getBlocks(glm::vec3(tempPos), glm::vec3(tempPos + glm::vec4(0, length, 0, 1) *
                                                                      (glm::rotate(glm::mat4(1.f), glm::radians(tempX), glm::vec3(0, 0, 1))) *
                                                                                          glm::rotate(glm::mat4(1.f), glm::radians(tempZ), glm::vec3(1, 0, 0))))) {
                if (!this->hasChunkAt(ref.x, ref.z)) return;

                if (buildMap.find(this->getChunkAt(ref.x, ref.z).get()) == buildMap.end()) {
                    buildMap.insert({this->getChunkAt(ref.x, ref.z).get(), {{ref}, {}}});
                } else {
                    buildMap.at(this->getChunkAt(ref.x, ref.z).get()).first.push_back(ref);
                }
            }
            tempPos += glm::vec4(0.f, length, 0.f, 1.f) *
                                               (glm::rotate(glm::mat4(1.f), glm::radians(tempX), glm::vec3(0.f, 0.f, 1.f))) *
                                               glm::rotate(glm::mat4(1.f), glm::radians(tempZ), glm::vec3(1.f, 0.f, 0.f));
            break;
        case TB:

            for (auto& ref : getBlocks(glm::vec3(tempPos), glm::vec3(tempPos) + glm::vec3(0, 1, 0))) {
                if (!this->hasChunkAt(ref.x, ref.z)) return;

                if (buildMap.find(this->getChunkAt(ref.x, ref.z).get()) == buildMap.end()) {
                    buildMap.insert({this->getChunkAt(ref.x, ref.z).get(), {{ref}, {}}});
                } else {
                    buildMap.at(this->getChunkAt(ref.x, ref.z).get()).first.push_back(ref);
                }
            }
            tempPos += glm::vec4(0.f, 1.f, 0.f, 0.f);
            break;
        case B:
            break;
        case L:
            /*for (int i = -1; i < 2; i += 2) {
                for (int j = -1; j < 2; j += 2) {
                    if (!this->hasChunkAt(tempPos.x + i, tempPos.z + j)) return;

                    if (buildMap.find(this->getChunkAt(tempPos.x + i, tempPos.z + j).get()) == buildMap.end()) {
                        buildMap.insert({this->getChunkAt(tempPos.x + i, tempPos.z + j).get(), {{}, {glm::ivec4(tempPos)}}});
                    } else {
                        buildMap.at(this->getChunkAt(tempPos.x + i, tempPos.z + j).get()).second.push_back(glm::ivec4(tempPos));
                    }
                }
            }*/
            break;
        case LT:
            for (int x = -3; x < 4; x++) {
                for (int y = -3; y < 4; y++) {
                    for (int z = -3; z < 4; z++) {
                        glm::vec4 curr = tempPos + glm::vec4(x, y, z, 0);
                        if (sdSphere(glm::vec3(x, y, z), 3.f) < 0.f) {
                            if (!this->hasChunkAt(curr.x, curr.z)) return;

                            if (buildMap.find(this->getChunkAt(curr.x, curr.z).get()) == buildMap.end()) {
                                buildMap.insert({this->getChunkAt(curr.x, curr.z).get(), {{}, {glm::ivec4(curr)}}});
                            } else {
                                buildMap.at(this->getChunkAt(curr.x, curr.z).get()).second.push_back(glm::ivec4(curr));
                            }
                        }
                    }
                }
            }
            break;
        case PX:
            tempX += 45;
            break;
        case NX:
            tempX -= 45;
            break;
        case PZ:
            tempZ += 45;
            break;
        case NZ:
            tempZ -= 45;
            break;
        case SA:
            turtleStack.push(turtle);
            break;
        case LD:
            turtle = turtleStack.top();
            turtleStack.pop();
            break;
        }
    }
    for (auto& x : buildMap) {
        for (auto& log : x.second.first) {
            this->setBlockAt(log.x, log.y, log.z, OAK_LOG);
        }
        for (auto& leaf : x.second.second) {
            this->setBlockAt(leaf.x, leaf.y, leaf.z, OAK_LEAVES);
        }
        x.first->resetVBOData();
    }
}

void multithread_work(std::vector<Chunk*>& chunks_to_generate, std::mutex& chunks_to_generate_mutex,
                      std::vector<Chunk*> &chunks_to_create, std::mutex &chunks_to_create_mutex,
                      std::vector<Chunk*>& chunks_to_save, std::mutex& chunks_to_save_mutex,
                      std::vector<ChunkVBOData>& created_chunks, std::mutex& created_chunks_mutex,
                      std::unordered_set<int64_t>& saved_chunks, std::mutex& saved_chunks_mutex,
                      QString& savename, std::atomic_bool& idle, std::atomic_bool& terminate)
{
    // Continuously loop, looking for new inputs
    while (true)
    {
        int workDone = 0;
        workDone += generate_chunks(chunks_to_generate, chunks_to_generate_mutex,
                        chunks_to_create, chunks_to_create_mutex,
                        saved_chunks, saved_chunks_mutex, savename);
        workDone += create_chunks(chunks_to_create, chunks_to_create_mutex,
                      created_chunks, created_chunks_mutex);
        workDone += save_chunks(chunks_to_save,chunks_to_save_mutex, savename);

        idle.store(!workDone);
        if (terminate.load() && !workDone) break;
    }
}

int generate_chunks(std::vector<Chunk*>& chunks_to_generate, std::mutex& chunks_to_generate_mutex,
                     std::vector<Chunk*>& chunks_to_create, std::mutex& chunks_to_create_mutex,
                     std::unordered_set<int64_t>& saved_chunks, std::mutex& saved_chunks_mutex, QString& savename)
{
    int workDone = 0;
    // Generate blocktype data for each chunk in the given input vector
    chunks_to_generate_mutex.lock();
    for (Chunk* c : chunks_to_generate)
    {
        saved_chunks_mutex.lock();
        bool hasFile = !(saved_chunks.find(c->getKey()) == saved_chunks.end());
        saved_chunks_mutex.unlock();

        if (hasFile) {
            c->load(savename);
        } else {
            c->setChunkGenHeights();
        }

        if (c->canCreate()){
            chunks_to_create_mutex.lock();
            chunks_to_create.push_back(c);
            chunks_to_create_mutex.unlock();
        }

        workDone++;
    }
    chunks_to_generate.clear();
    chunks_to_generate_mutex.unlock();
    return workDone;
}


int create_chunks(std::vector<Chunk*>& chunks_to_create, std::mutex& chunks_to_create_mutex,
                   std::vector<ChunkVBOData>& created_chunks, std::mutex& created_chunks_mutex)
{
    int workDone = 0;
    // Generate VBO data for each chunk in the given input vector
    chunks_to_create_mutex.lock();
    for (Chunk* c : chunks_to_create)
    {
        std::vector<glm::vec4> vboOpaque, vboTransparent;
        std::vector<GLuint> idxOpaque, idxTransparent;
        c->makeDrawableVBOs(vboOpaque, idxOpaque, vboTransparent, idxTransparent);
        ChunkVBOData c_data = ChunkVBOData(c);
        c_data.idxDataOpaque = idxOpaque;
        c_data.idxDataTransparent = idxTransparent;
        c_data.vboDataOpaque = vboOpaque;
        c_data.vboDataTransparent = vboTransparent;

        created_chunks_mutex.lock();
        created_chunks.push_back(c_data);
        created_chunks_mutex.unlock();
        workDone++;
    }
    chunks_to_create.clear();
    chunks_to_create_mutex.unlock();
    return workDone;
}

int save_chunks(std::vector<Chunk*>& chunks_to_save, std::mutex& chunks_to_save_mutex, QString& savename) {
    int workDone = 0;
    chunks_to_save_mutex.lock();
    for (Chunk* c : chunks_to_save) {
        c->save(savename);
        workDone++;
    }
    chunks_to_save.clear();
    chunks_to_save_mutex.unlock();
    return workDone;
}

void Terrain::load(QString savename) {
    this->savename = savename;
    //neededProgress = (16 * CREATE_RADIUS * CREATE_RADIUS * 2) - 1;

    QFile file("../saves/"+savename+"/"+savename+".save");
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    // load prevous player posiition and seed
    in >> prev_pos.x >> prev_pos.y >> prev_pos.z;
    in >> seed;
    TerrainGen::setSeed(seed);

    while (!in.atEnd()) {
        int64_t chunkKey;
        in >> chunkKey;
        saved.insert(chunkKey);
    }

    file.close();

    threadsAreNotIdle();
}

void Terrain::save() {
    int i = 0;
    for (Chunk* chunk : updated) {
        saved.insert(chunk->getKey());
        chunks_needing_saving_mutexes[i].lock();
        chunks_needing_saving[i].push_back(chunk);
        chunks_needing_saving_mutexes[i].unlock();
        i = (i + 1) % NUM_CORES;
    }

    QFile file("../saves/"+savename+"/"+savename+".save");
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    out << prev_pos.x << prev_pos.y << prev_pos.z;
    out << seed;

    for(int64_t chunk : saved) {
        out << chunk;
    }

    file.close();

    threadsAreNotIdle();
}

void Terrain::threadsAreNotIdle() {
    for (int i = 0; i < NUM_CORES; i++) {
        idle_threads[i].store(false);
    }
}

bool Terrain::threadsIdle() {
    bool idle = true;
    for (int i = 0; i < NUM_CORES; i++) {
        idle = idle && idle_threads[i].load();
    }
    return idle;
}
