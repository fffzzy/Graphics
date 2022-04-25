#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "drawable.h"
#include "chunkhelpers.h"
#include <array>
#include <unordered_map>
#include <cstddef>

enum CallerTypeC {
    SYSTEM_C, PLAYER_C
};

class Chunk;
//using namespace std;

struct ChunkVBOData {
    std::vector<glm::vec4> m_vboDataTransparent;
    std::vector<glm::vec4> m_vboDataOpaque;
    std::vector<GLuint> m_idxDataTransparent;
    std::vector<GLuint> m_idxDataOpaque;
    Chunk* mp_chunk;
    ChunkVBOData(Chunk* c): m_vboDataTransparent{}, m_vboDataOpaque{}, m_idxDataTransparent{}, m_idxDataOpaque{}, mp_chunk(c)
    {}
};


// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

// TODO have Chunk inherit from Drawable
class Chunk : public Drawable {
public:
    glm::ivec2 m_coords;
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

public:
    explicit Chunk(OpenGLContext* mp_context, int x, int z);
    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    BlockType getBlockAt(glm::vec3 pos) const;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t, CallerTypeC ct = SYSTEM_C);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);
    virtual void createVBOdata() override;
    //void bufferVBOdata(std::vector<glm::vec4> interleavedData, std::vector<int> indices);

    void bufferVBOdata(std::vector<glm::vec4> m_vboDataOpaque,
                       std::vector<GLuint> m_idxDataOpaque,
                       std::vector<glm::vec4> m_vboDataTransparent,
                       std::vector<GLuint> m_idxDataTransparent);
    ChunkVBOData m_chunkVBOData;
    bool hasVBOdata;
    void generateChunk();
    void setBlock(int x, int z); // sets blocks on coordinates x,z
    virtual void destroyVBOdata() override;
    float WorleyDist(glm::vec2 uv);
    float fbm(float x, float persistence = 0.5f, int octaves = 5, float freq = 2.f, float amp = 0.5f);
    float interpNoise1D(float x);
    float noise1D(int x);
    float perlinNoise(glm::vec2 uv);
    float perlinNoise2(glm::vec2 uv);
    float surflet(glm::vec2 P, glm::vec2 gridPoint);
    float surflet2(glm::vec2 P, glm::vec2 gridPoint);
    float surflet3D(glm::vec3 P, glm::vec3 gridPoint);
    float perlinNoise3D(glm::vec3 uv);
    float random1( glm::vec2 p );
    glm::vec2 random2( glm::vec2 p );
    glm::vec2 random2_2( glm::vec2 p );
    glm::vec3 random3( glm::vec3 p );
};
