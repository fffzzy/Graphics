#include "terrain.h"
#include "cube.h"
#include <stdexcept>
#include <iostream>
#include <math.h>
#include <algorithm>

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context),
      m_tryExpansionTimer(0.f)
{}

Terrain::~Terrain() {
    for (std::pair<const long long int, uPtr<Chunk>>& c : m_chunks) {
        c.second->destroyVBOdata();
    }
}

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
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    // Turn coordinates into multiples of 16
    x = 16 * glm::floor(x / 16.f);
    z = 16 * glm::floor(z / 16.f);

    // Instantiate chunk
    uPtr<Chunk> chunk = mkU<Chunk>(this->mp_context);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = move(chunk);
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

    // Populate blocks
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 16; j++){
            setBlock(x + i, z+ j);
        }
    }

    return cPtr;
    return cPtr;
}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!
void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram) {

    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            if (hasChunkAt(x, z)) {
                const uPtr<Chunk> &chunk = getChunkAt(x, z);
                if(!chunk->hasVBOdata) {
                    continue;
                }
                // Set model matrix to appropriate offset
                glm::mat4 modelMatrix = glm::mat4(1.f);
                modelMatrix[3][0] = x;
                modelMatrix[3][2] = z;
                shaderProgram->setModelMatrix(modelMatrix);
                chunk->createVBOdata();
                shaderProgram->drawInterleaved(*chunk.get());
            }
        }
    }
}


glm::vec2 random2( glm::vec2 p ) {
    return glm::fract(glm::sin(glm::vec2(glm::dot(p, glm::vec2(127.1, 311.7)),
                 glm::dot(p, glm::vec2(269.5,183.3))))
                 * (float)43758.5453);
}

float surflet(glm::vec2 P, glm::vec2 gridPoint) {
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

float perlinNoise(glm::vec2 uv) {
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            surfletSum += surflet(uv, glm::floor(uv) + glm::vec2(dx, dy));
        }
    }
    return surfletSum;
}


float noise1D(int x) {
    double intPart, fractPart;
    fractPart = std::modf(sin(x*127.1) *
            43758.5453, &intPart);
    return fractPart;
}



float interpNoise1D(float x) {
    int intX = int(floor(x));
    float fractX = x - intX;

    float v1 = noise1D(intX);
    float v2 = noise1D(intX+1);
    return v1 + fractX*(v2-v1);
}

float fbm(float x) {
    float total = 0;
    float persistence = 0.5f;
    int octaves = 8;
    float freq = 2.f;
    float amp = 0.5f;
    for(int i = 1; i <= octaves; i++) {
        total += interpNoise1D(x * freq) * amp;

        freq *= 2.f;
        amp *= persistence;
    }
    return total;
}

float WorleyDist(glm::vec2 uv) {
    float grid = 2.0;
    uv *= grid; // Now the space is 10x10 instead of 1x1. Change this to any number you want.
    glm::vec2 uvInt = glm::floor(uv);
    glm::vec2 uvFract = glm::fract(uv);

    float minDist = 1000; // Minimuxm distance initialized to max.
    glm::vec2 minPoint = glm::vec2(200,200);
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            glm::vec2 neighbor = glm::vec2(float(x), float(y)); // Direction in which neighbor cell lies
            glm::vec2 point = random2(uvInt + neighbor); // Get the Voronoi centerpoint for the neighboring cell
            glm::vec2 diff = neighbor + point - uvFract; // Distance between fragment coord and neighbor’s Voronoi point
            float dist = glm::length(diff);
            if(dist < minDist){
                minPoint = glm::vec2((uv+diff)/grid);
            }
            minDist = std::min(minDist, dist);
        }
    }
    return minDist;
}

void Terrain::setBlock(int x, int z){
    float b = perlinNoise(glm::vec2(x/300.0, z/300.0))+0.5;

    float p = (perlinNoise(glm::vec2(x/64.0 ,z/64.0) ) + 0.5);
    float r = fbm(p);
    float m = -508*r + 203.2 ;

    m = std::max(std::min(
                     m,127.f),0.f); // mountain height

    m+=128;

    float w = WorleyDist(glm::vec2(x/64.0 ,z/64.0));
    float g = -25*w + 25;

    g = std::max(std::min(
                     g,40.f),0.f); // hill height

    g+=128;

    int f;

    if(b > 0.6){
        f = int(m);
    }else if (b < 0.4){
        f = int(g);
    }else{
        f = int(glm::mix(g, m, b));
    }

    f = std::max(std::min(
                     f,254),0); // interpolated value


    //comment this out to run faster
    for(int i = 1; i <= 128; i++){
        setBlockAt(x, i, z, STONE); // set stone underground
    }

    if(b > 0.5){
        for(int i = 129; i <= f; i++){
            if(i == f && f >= 200){
                setBlockAt(x, i, z, SNOW); // top of mountain
            }else{
                setBlockAt(x, i, z, STONE); // set mountains stone
            }
        }

    }
    else{
        for(int i = 129; i <= f; i++){
            if(i == f){
                setBlockAt(x, i, z, GRASS); // top of hills
            }else{
                setBlockAt(x, i, z, DIRT); // set hills dirt
            }
        }
    }
    for(int i = f; i < 138; i++){
        setBlockAt(x, i, z, WATER); // water 128 - 138
    }
}


void Terrain::CreateTestScene()
{
    // TODO: DELETE THIS LINE WHEN YOU DELETE m_geomCube!
    //m_geomCube.createVBOdata();

    // Create the Chunks that will
    // store the blocks for our
    // initial world space
    for(int x = 0; x < 64; x += 16) {
        for(int z = 0; z < 64; z += 16) {
            instantiateChunkAt(x, z);
        }
    }
    // Tell our existing terrain set that
    // the "generated terrain zone" at (0,0)
    // now exists.
    m_generatedTerrain.insert(toKey(0, 0));

    // Create the basic terrain floor
    for(int x = 0; x < 64; ++x) {
        for(int z = 0; z < 64; ++z) {
            setBlock(x,z);
        }
    }

}

void Terrain::spawnVBOWorkers(const vector<Chunk*> &chunksNeedingVBOs) {
    for (Chunk* c : chunksNeedingVBOs) {
        spawnVBOWorker(c);
    }
}

void Terrain::checkThreadResults() {
    m_chunksThatHaveBlockDataLock.lock();
    spawnVBOWorkers(m_chunksThatHaveBlockData);
    m_chunksThatHaveBlockData.clear();
    m_chunksThatHaveBlockDataLock.unlock();

    m_chunksThatHaveVBOsLock.lock();
    for(auto& cd: m_chunksThatHaveVBOs) {
        cd.mp_chunk->bufferVBOdata(cd.m_vboDataOpaque, cd.m_idxDataOpaque,
                                   cd.m_vboDataTransparent, cd.m_idxDataTransparent);
        cd.mp_chunk->hasVBOdata = true;
    }
    m_chunksThatHaveVBOs.clear();
    m_chunksThatHaveVBOsLock.unlock();
}

void Terrain::multithreadedWork(glm::vec3 playerPos, glm::vec3 playerPosPrev, float dT) {
    m_tryExpansionTimer += dT;
    if (m_tryExpansionTimer < 5.f) {
        return;
    }
    tryExpansion(playerPos, playerPosPrev);
    checkThreadResults();
    m_tryExpansionTimer = 0.f;
}

void Terrain::tryExpansion(glm::vec3 playerPos, glm::vec3 playerPosPrev) {
    glm::ivec2 currZone = glm::ivec2(glm::floor(playerPos.x / 64.f) * 64.f, glm::floor(playerPos.z / 64.f) * 64.f);
    glm::ivec2 prevZone = glm::ivec2(glm::floor(playerPosPrev.x / 64.f) * 64.f, glm::floor(playerPosPrev.z / 64.f) * 64.f);

    QSet<int64_t> terrainZonesBorderingCurrPos = terrainZonesBoarderingZone(currZone);
    QSet<int64_t> terrainZonesBorderingPrevPos = terrainZonesBoarderingZone(prevZone);

    //destroy
    for(auto id: terrainZonesBorderingPrevPos) {
        if(!terrainZonesBorderingCurrPos.contains(id)) {
            glm::ivec2 coord = toCoords(id);
            for(int x = coord.x; x < coord.x + 64; x += 16) {
                for(int z = coord.y; z < coord.y + 64; z += 16) {
                    auto& chunk = getChunkAt(x, z);
                    chunk->destroyVBOdata();
                }
            }
        }
    }
    for(auto id: terrainZonesBorderingCurrPos) {
        glm::ivec2 zone = toCoords(id);
        if(terrainZoneExists(zone.x,zone.y)) {
            if(!terrainZonesBorderingPrevPos.contains(id)) {
                for(int x = zone.x; x < zone.x + 64; x += 16) {
                    for(int z = zone.y; z < zone.y + 64; z += 16) {
                        auto& chunk = getChunkAt(x, z);
                        spawnVBOWorker(chunk.get());
                    }
                }
            }
        }else{
            spawnBlockTypeWorker(id);
        }
    }
}

QSet<int64_t> Terrain::terrainZonesBoarderingZone(glm::ivec2 zone) {
    QSet<int64_t> neighbors;
    //5x5
    for (int i = -128; i <= 128; i += 64) {
        for (int j = -128; j <= 128; j += 64) {
            neighbors.insert(toKey(zone.x + i, zone.y + j));
        }
    }
    return neighbors;
}

bool Terrain::terrainZoneExists(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 64.f));
    int zFloor = static_cast<int>(glm::floor(z / 64.f));
    return m_generatedTerrain.find(toKey(64 * xFloor, 64 * zFloor)) != m_generatedTerrain.end();
}

void Terrain::spawnBlockTypeWorker(int64_t zoneToGenerate) {
    m_generatedTerrain.insert(zoneToGenerate);
    vector<Chunk*> chunksforWorker;
    glm::ivec2 coords = toCoords(zoneToGenerate);
    for(int x = coords.x; x < coords.x + 64; x += 16) {
        for(int z = coords.y; z < coords.y + 64; z += 16) {
            Chunk* c = instantiateChunkAt(x,z);
            // c->m_countOpq = 0; //allow it to be drawn even without VBO data
            // c->m_countTra = 0; //allow it to be drawn even without VBO data
            chunksforWorker.push_back(c);
        }
    }
    BlockTypeWorker *worker = new BlockTypeWorker(coords.x, coords.y, chunksforWorker, &m_chunksThatHaveBlockData, &m_chunksThatHaveBlockDataLock);
    QThreadPool::globalInstance()->start(worker);
}

void Terrain::spawnVBOWorker(Chunk* chunkNeedingVBOData) {
    VBOWorker *worker = new VBOWorker(chunkNeedingVBOData, &m_chunksThatHaveVBOs, &m_chunksThatHaveVBOsLock);
    QThreadPool::globalInstance()->start(worker);
}



