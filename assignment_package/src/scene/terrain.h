#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "chunk.h"
#include <array>
#include <unordered_map>
#include <unordered_set>
#include "shaderprogram.h"

#include <thread>
#include <mutex>
#include <atomic>

#include <scene/terraingen.h>

#include "tree.h"

// We will designate NUM_CORES threads for processing blocktype data and VBO data
// (my cpu has 6 cores, so i set it to 6)

#define NUM_CORES 5

#define CREATE_RADIUS 2
#define DRAW_RADIUS 1

//using namespace std;

// Helper functions to convert (x, z) to and from hash map key
int64_t toKey(int x, int z);
glm::ivec2 toCoords(int64_t k);

// The container class for all of the Chunks in the game.
// Ultimately, while Terrain will always store all Chunks,
// not all Chunks will be drawn at any given time as the world
// expands.
class Terrain {
private:
    // Stores every Chunk according to the location of its lower-left corner
    // in world space.
    // We combine the X and Z coordinates of the Chunk's corner into one 64-bit int
    // so that we can use them as a key for the map, as objects like std::pairs or
    // glm::ivec2s are not hashable by default, so they cannot be used as keys.
    std::unordered_map<int64_t, uPtr<Chunk>> m_chunks;

    // We will designate every 64 x 64 area of the world's x-z plane
    // as one "terrain generation zone". Every time the player moves
    // near a portion of the world that has not yet been generated
    // (i.e. its lower-left coordinates are not in this set), a new
    // 4 x 4 collection of Chunks is created to represent that area
    // of the world.
    // The world that exists when the base code is run consists of exactly
    // one 64 x 64 area with its lower-left corner at (0, 0).
    // When milestone 1 has been implemented, the Player can move around the
    // world to add more "terrain generation zone" IDs to this set.
    // While only the 3 x 3 collection of terrain generation zones
    // surrounding the Player should be rendered, the Chunks
    // in the Terrain will never be deleted until the program is terminated.
    std::unordered_set<int64_t> m_generatedTerrain;

    OpenGLContext* mp_context;

    // Set which will contain the terrain and chunk data generated/created by the BlockTypeWorkers/VBOWorkers
    std::array<std::vector<Chunk*>, NUM_CORES> chunks_needing_generated;
    std::array<std::mutex, NUM_CORES> chunks_needing_generated_mutexes;

    std::array<std::vector<Chunk*>, NUM_CORES> chunks_needing_created;
    std::array<std::mutex, NUM_CORES> chunks_needing_created_mutexes;

    std::vector<ChunkVBOData> created_chunks;
    std::mutex created_chunks_mutex;

    // Vector of threads representing both our BlockType Workers and VBOWorkers
    std::vector<std::thread> threads;

    // Timer for generating terrain; since the player is not going to enter a new
    // zone every tick, we can make things more efficient by firing
    // Terrain::tick() once every second or so
    float terrain_timer;

    // vec3 containing position of the player when we last tried to load new terrain
    glm::vec3 prev_pos;

    std::vector<uPtr<Tree>> trees;

    // for loading
    std::unordered_set<int64_t> saved;
    std::mutex savedMutex;
    std::unordered_set<Chunk*> updated;
    QString savename;
    std::array<std::vector<Chunk*>, NUM_CORES> chunks_needing_saving;
    std::array<std::mutex, NUM_CORES> chunks_needing_saving_mutexes;

    int seed;

    std::array<std::atomic_bool,NUM_CORES> idle_threads;
    std::atomic_bool terminate;

public:
    Terrain(OpenGLContext *context);
    Terrain(OpenGLContext *context, QString savename);
    ~Terrain();

    // Instantiates a new Chunk and stores it in
    // our chunk map at the given coordinates.
    // Returns a pointer to the created Chunk.
    Chunk* instantiateChunkAt(int x, int z);
    // Do these world-space coordinates lie within
    // a Chunk that exists?
    bool hasChunkAt(int x, int z) const;
    // Assuming a Chunk exists at these coords,
    // return a mutable reference to it
    uPtr<Chunk>& getChunkAt(int x, int z);
    // Assuming a Chunk exists at these coords,
    // return a const reference to it
    const uPtr<Chunk>& getChunkAt(int x, int z) const;
    // Given a world-space coordinate (which may have negative
    // values) return the block stored at that point in space.
    BlockType getBlockAt(int x, int y, int z) const;
    BlockType getBlockAt(glm::vec3 p) const;
    // Given a world-space coordinate (which may have negative
    // values) set the block at that point in space to the
    // given type.
    void setBlockAt(int x, int y, int z, BlockType t);

    // Draws every Chunk that falls within the bounding box
    // described by the min and max coords, using the provided
    // ShaderProgram
    void draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram);

    // Creates new chunks when the player is within 16 blocks of an edge of a chunk that does not connect to an existing chunk
    void expandTerrain(const glm::vec3 &player_pos);

    // Function called every tick, checking for terrain updates based on player_pos
    void tick(const glm::vec3 &player_pos, float dt);

    // Function for creating BlockType Workers for terrain expansion (and deloading of chunks)
    void tryExpansion(const glm::vec3 &player_pos, const glm::vec3 &prev_pos);

    // Function for creating VBO Workers for terrain creation
    void createTerrain();

    // Helper function for unloading chunks
    void deleteTerrain(int64_t &key);

    void createTree(glm::ivec4& pos);

    void growTrees(qint64 currTime);
    
    void buildTree(glm::ivec4& pos, std::vector<TreeSymbol>& axiom);
  
    void save();
    void load(QString savename);

    bool threadsIdle();
    void threadsAreNotIdle();
};

// Function for our BlockTypeWorker's to continuously run
// waiting for ivec2's representing chunks to be generated at
// the given two x and z coordinates
int generate_chunks(std::vector<Chunk*>& chunks_to_generate, std::mutex& chunks_to_generate_mutex,
                     std::vector<Chunk*>& chunks_to_create, std::mutex& chunks_to_create_mutex,
                     std::unordered_set<int64_t>& saved_chunks, std::mutex& saved_chunks_mutex, QString& savename);

// Function for our VBOWorkers to continuously run
// waiting for Chunk*'s to create the VBO data
int create_chunks(std::vector<Chunk*> &chunks_to_create, std::mutex& chunks_to_create_mutex,
                   std::vector<ChunkVBOData>& created_chunks, std::mutex& created_chunks_mutex);

int save_chunks(std::vector<Chunk*>& chunks_to_save, std::mutex& chunks_to_save_mutex, QString& savename);

// Function for our threads to continuously run
// Does both the work for blocktype data and VBO data in one function
void multithread_work(std::vector<Chunk*>& chunks_to_generate, std::mutex& chunks_to_generate_mutex,
                      std::vector<Chunk*> &chunks_to_create, std::mutex &chunks_to_create_mutex,
                      std::vector<Chunk*>& chunks_to_save, std::mutex& chunks_to_save_mutex,
                      std::vector<ChunkVBOData>& created_chunks, std::mutex& created_chunks_mutex,
                      std::unordered_set<int64_t>& saved_chunks, std::mutex& saved_chunks_mutex,
                      QString& savename, std::atomic_bool& idle, std::atomic_bool& terminate);
