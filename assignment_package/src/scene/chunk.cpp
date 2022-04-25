#include "chunk.h"
#include <iostream>

Chunk::Chunk(OpenGLContext* mp_context, int x, int z) : Drawable(mp_context),
    m_coords(x, z), m_blocks(),
    m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}},
    m_chunkVBOData(this), hasVBOdata(false)
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

void Chunk::destroyVBOdata() {
    Drawable::destroyVBOdata();
    this->hasVBOdata = false;
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    // Check for limits
    if ((int) x < 0) {
        // Check for no neighbor
        if (m_neighbors.at(XNEG) == nullptr) {
            return UNDETERMINED;
        }
        return m_neighbors.at(XNEG)->getBlockAt(16 + x, y, z);
    } else if ((int) x >= 16) {
        // Check for no neighbor
        if (m_neighbors.at(XPOS) == nullptr) {
            return UNDETERMINED;
        }
        return m_neighbors.at(XPOS)->getBlockAt(x - 16, y, z);
    } else if ((int) y < 0) {
        // Check for no neighbor
        if (m_neighbors.at(YNEG) == nullptr) {
            return UNDETERMINED;
        }
        return m_neighbors.at(YNEG)->getBlockAt(x, 256 + y, z);
    } else if ((int) y >= 256) {
        // Check for no neighbor
        if (m_neighbors.at(YPOS) == nullptr) {
            return UNDETERMINED;
        }
        return m_neighbors.at(YPOS)->getBlockAt(x, y - 256, z);
    } else if ((int) z < 0) {
        // Check for no neighbor
        if (m_neighbors.at(ZNEG) == nullptr) {
            return UNDETERMINED;
        }
        return m_neighbors.at(ZNEG)->getBlockAt(x, y, 16 + z);
    } else if ((int) z >= 16) {
        // Check for no neighbor
        if (m_neighbors.at(ZPOS) == nullptr) {
            return UNDETERMINED;
        }
        return m_neighbors.at(ZPOS)->getBlockAt(x, y, z - 16);
    }

    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
    //return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to simplify calls by allowing a vector argument
BlockType Chunk::getBlockAt(glm::vec3 pos) const {
    //return getBlockAt(static_cast<unsigned int>(pos.x), static_cast<unsigned int>(pos.y), static_cast<unsigned int>(pos.z));
    return getBlockAt(static_cast<unsigned int>(pos.x), static_cast<unsigned int>(pos.y), static_cast<unsigned int>(pos.z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t, CallerTypeC ct) {
    m_blocks.at((x % 16) + 16 * (y % 256) + 16 * 256 * (z % 16)) = t;
    if (ct == PLAYER_C) {
        this->destroyVBOdata();
        this->createVBOdata();
        this->bufferVBOdata(this->m_chunkVBOData.m_vboDataOpaque, this->m_chunkVBOData.m_idxDataOpaque, this->m_chunkVBOData.m_vboDataTransparent, this->m_chunkVBOData.m_idxDataTransparent);
        this->hasVBOdata = true;
    }
}


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

void Chunk::createVBOdata() {
    // Create stores for all the opaque square faces to be drawn
    std::vector<glm::vec4> O_pos = std::vector<glm::vec4>();
    std::vector<glm::vec4> O_nor = std::vector<glm::vec4>();
    std::vector<glm::vec4> O_uv = std::vector<glm::vec4>();
    std::vector<GLuint> O_idx = std::vector<GLuint>();

    // Create stores for all the transparent square faces to be drawn
    std::vector<glm::vec4> T_pos = std::vector<glm::vec4>();
    std::vector<glm::vec4> T_nor = std::vector<glm::vec4>();
    std::vector<glm::vec4> T_uv = std::vector<glm::vec4>();
    std::vector<GLuint> T_idx = std::vector<GLuint>();

    int O_indexOffset = 0;
    int T_indexOffset = 0;

    // Iterate through all the blocks
    for (int z = 0; z < 16; z++) {
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 16; x++) {
                BlockType btAtCurrPos = getBlockAt(x, y, z);

                if (btAtCurrPos != EMPTY) {
                    glm::vec3 currPos = glm::vec3(x, y, z);
                    glm::vec3 currWorldPos = glm::vec3(x, y, z);

                    // Look at all neighbors and add appropriate faces
                    for (BlockFace neighborFace : adjacentFaces) {
                        BlockType neighborType = getBlockAt(neighborFace.directionVec + currPos);

                        // If the neighbor is empty, not undetermined and this block is a non liquid block touching liquid, add vertices to collection
                        if (neighborType == EMPTY || ((neighborType == WATER || neighborType == LAVA) && (btAtCurrPos != WATER && btAtCurrPos != LAVA))) {
                            for (VertexData VD : neighborFace.vertices) {
                                glm::vec2 UVoffset;

                                switch(btAtCurrPos) {
                                    case GRASS:
                                        // Set offset for grass top
                                        if (neighborFace.direction == YPOS) {
                                            UVoffset = glm::vec2(8, 13);
                                        } else if (neighborFace.direction == YNEG) { // Set offset for grass bottom
                                            UVoffset = glm::vec2(2, 15);
                                        } else { // Set offset for grass sides
                                            UVoffset = glm::vec2(3, 15);
                                        }
                                        break;
                                    case DIRT:
                                        // Set offset for dirt
                                        UVoffset = glm::vec2(2, 15);
                                        break;
                                    case STONE:
                                        // Set offset for stone
                                        UVoffset = glm::vec2(1, 15);
                                        break;
                                    case SNOW:
                                        UVoffset = glm::vec2(2, 11);
                                        break;
                                    case WATER:
                                        UVoffset = glm::vec2(13, 3);
                                        break;
                                    case LAVA:
                                        UVoffset = glm::vec2(13, 1);
                                        break;
                                    case BEDROCK:
                                        UVoffset = glm::vec2(1, 15);
                                        break;
                                    case SAND:
                                        UVoffset = glm::vec2(2, 14);
                                        break;
                                    case MOSS_STONE:
                                        UVoffset = glm::vec2(4, 13);
                                        break;
                                    default:
                                        // Other block types are not yet handled, so we default to debug purple
                                        UVoffset = glm::vec2(10, 3);
                                        break;
                                }

                                // Push VBO Data
                                if (btAtCurrPos != WATER && btAtCurrPos != LAVA) {
                                    O_pos.push_back(glm::vec4(currWorldPos, 0.f) + VD.pos);
                                    O_nor.push_back(glm::vec4(neighborFace.directionVec, 0.f));
                                    O_uv.push_back(glm::vec4(VD.uv + UVoffset / 16.f, 0, 0));
                                } else {
                                    T_pos.push_back(glm::vec4(currWorldPos, 0.f) + VD.pos);
                                    T_nor.push_back(glm::vec4(neighborFace.directionVec, 0.f));
                                    T_uv.push_back(glm::vec4(VD.uv + UVoffset / 16.f, 0, 0));
                                }
                            }

                            // Push indices
                            if (btAtCurrPos != WATER && btAtCurrPos != LAVA) {
                                O_idx.push_back(0 + O_indexOffset);
                                O_idx.push_back(1 + O_indexOffset);
                                O_idx.push_back(2 + O_indexOffset);
                                O_idx.push_back(0 + O_indexOffset);
                                O_idx.push_back(2 + O_indexOffset);
                                O_idx.push_back(3 + O_indexOffset);
                                O_indexOffset += 4;
                            } else {
                                T_idx.push_back(0 + T_indexOffset);
                                T_idx.push_back(1 + T_indexOffset);
                                T_idx.push_back(2 + T_indexOffset);
                                T_idx.push_back(0 + T_indexOffset);
                                T_idx.push_back(2 + T_indexOffset);
                                T_idx.push_back(3 + T_indexOffset);
                                T_indexOffset += 4;
                            }
                        }
                    }
                }
            }
        }
    }

    // Interleave vectors
    std::vector<glm::vec4> O_interleavedVector = std::vector<glm::vec4>();
    std::vector<glm::vec4> T_interleavedVector = std::vector<glm::vec4>();

    for (int i = 0; i < O_pos.size(); i++) {
        O_interleavedVector.push_back(O_pos[i]);
        O_interleavedVector.push_back(O_nor[i]);
        O_interleavedVector.push_back(O_uv[i]);
    }
    for (int i = 0; i < T_pos.size(); i++) {
        T_interleavedVector.push_back(T_pos[i]);
        T_interleavedVector.push_back(T_nor[i]);
        T_interleavedVector.push_back(T_uv[i]);
    }

    this->m_chunkVBOData.m_idxDataOpaque = O_idx;
    this->m_chunkVBOData.m_vboDataOpaque = O_interleavedVector;
    this->m_chunkVBOData.m_idxDataTransparent = T_idx;
    this->m_chunkVBOData.m_vboDataTransparent = T_interleavedVector;
}

void Chunk::bufferVBOdata(std::vector<glm::vec4> m_vboDataOpaque,
                          std::vector<GLuint> m_idxDataOpaque,
                          std::vector<glm::vec4> m_vboDataTransparent,
                          std::vector<GLuint> m_idxDataTransparent) {
    this->m_count = m_idxDataOpaque.size();
    this->m_count_sec = m_idxDataTransparent.size();

    generatePos();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    mp_context->glBufferData(GL_ARRAY_BUFFER, m_vboDataOpaque.size() * sizeof(glm::vec4), m_vboDataOpaque.data(), GL_STATIC_DRAW);

    generateNor();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufNor);
    mp_context->glBufferData(GL_ARRAY_BUFFER, m_vboDataOpaque.size() * sizeof(glm::vec4), m_vboDataOpaque.data(), GL_STATIC_DRAW);

    generateUV();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUV);
    mp_context->glBufferData(GL_ARRAY_BUFFER, m_vboDataOpaque.size() * sizeof(glm::vec4), m_vboDataOpaque.data(), GL_STATIC_DRAW);

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_idxDataOpaque.size() * sizeof(GLuint), m_idxDataOpaque.data(), GL_STATIC_DRAW);

    generatePos_sec();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos_sec);
    mp_context->glBufferData(GL_ARRAY_BUFFER, m_vboDataTransparent.size() * sizeof(glm::vec4), m_vboDataTransparent.data(), GL_STATIC_DRAW);

    generateNor_sec();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufNor_sec);
    mp_context->glBufferData(GL_ARRAY_BUFFER, m_vboDataTransparent.size() * sizeof(glm::vec4), m_vboDataTransparent.data(), GL_STATIC_DRAW);

    generateUV_sec();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUV_sec);
    mp_context->glBufferData(GL_ARRAY_BUFFER, m_vboDataTransparent.size() * sizeof(glm::vec4), m_vboDataTransparent.data(), GL_STATIC_DRAW);

    generateIdx_sec();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx_sec);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_idxDataTransparent.size() * sizeof(GLuint), m_idxDataTransparent.data(), GL_STATIC_DRAW);
}
void Chunk::generateChunk(){
    // Populate blocks
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 16; j++){
            setBlock(this->m_coords.x + i, this->m_coords.y + j);
        }
    }
}

glm::vec2 Chunk::random2( glm::vec2 p ) {
    return glm::fract(glm::sin(glm::vec2(glm::dot(p, glm::vec2(127.1, 311.7)),
                 glm::dot(p, glm::vec2(269.5,183.3))))
                 * (float)43758.5453);
}

glm::vec2 Chunk::random2_2( glm::vec2 p ) {
    return glm::fract(glm::sin(glm::vec2(glm::dot(p, glm::vec2(156.1, 287.7)),
                 glm::dot(p, glm::vec2(412.5,984.3))))
                 * (float)37295.5453);
}

float Chunk::surflet(glm::vec2 P, glm::vec2 gridPoint) {
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

float Chunk::surflet2(glm::vec2 P, glm::vec2 gridPoint) {
    // Compute falloff function by converting linear distance to a polynomial
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float tX = 1 - 6 * pow(distX, 5.f) + 15 * pow(distX, 4.f) - 10 * pow(distX, 3.f);
    float tY = 1 - 6 * pow(distY, 5.f) + 15 * pow(distY, 4.f) - 10 * pow(distY, 3.f);
    // Get the random vector for the grid point
    glm::vec2 gradient = 2.f * random2_2(gridPoint) - glm::vec2(1.f);
    // Get the vector from the grid point to P
    glm::vec2 diff = P - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = glm::dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY;
}

//perlin noise function, used in biome generation
float Chunk::perlinNoise(glm::vec2 uv) {
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            surfletSum += surflet(uv, glm::floor(uv) + glm::vec2(dx, dy));
        }
    }
    return surfletSum;
}

//perlin noise function, used in temperature biome generation
float Chunk::perlinNoise2(glm::vec2 uv) {
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            surfletSum += surflet2(uv, glm::floor(uv) + glm::vec2(dx, dy));
        }
    }
    return surfletSum;
}


float Chunk::noise1D(int x) {
    double intPart, fractPart;
    fractPart = std::modf(sin(x*127.1) *
            43758.5453, &intPart);
    return fractPart;
}



float Chunk::interpNoise1D(float x) {
    int intX = int(floor(x));
    float fractX = x - intX;

    float v1 = noise1D(intX);
    float v2 = noise1D(intX+1);
    return v1 + fractX*(v2-v1);
}
//fractial brownian noise used for mountain biomes
float Chunk::fbm(float x , float persistence, int octaves, float freq, float amp) {
    float total = 0;
    for(int i = 1; i <= octaves; i++) {
        total += interpNoise1D(x * freq) * amp;

        freq *= 2.f;
        amp *= persistence;
    }
    return total;
}

//Worley Distance function used for generating hill biomes
float Chunk::WorleyDist(glm::vec2 uv) {
    float grid = 0.3f;
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

//used by surflet3D
glm::vec3 Chunk::random3( glm::vec3 p ) {
    return glm::fract(glm::sin(glm::vec3(glm::dot(p, glm::vec3(127.1, 311.7,114.9)),
                 glm::dot(p, glm::vec3(269.5,183.3,341.7)),glm::dot(p, glm::vec3(315.2,123.8,235.5))))
                 * (float)43758.5453);
}

//used by perlinNoise3D
float Chunk::surflet3D(glm::vec3 P, glm::vec3 gridPoint) {
    // Compute falloff function by converting linear distance to a polynomial
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float distZ = abs(P.z - gridPoint.z);
    float tX = 1 - 6 * pow(distX, 5.f) + 15 * pow(distX, 4.f) - 10 * pow(distX, 3.f);
    float tY = 1 - 6 * pow(distY, 5.f) + 15 * pow(distY, 4.f) - 10 * pow(distY, 3.f);
    float tZ = 1 - 6 * pow(distZ, 5.f) + 15 * pow(distZ, 4.f) - 10 * pow(distZ, 3.f);
    // Get the random vector for the grid point
    glm::vec3 gradient = 2.f * random3(gridPoint) - glm::vec3(1.f);
    //cout << glm::to_string(random3(gridPoint));
    // Get the vector from the grid point to P
    glm::vec3 diff = P - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = glm::dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY * tZ;
}

//perlin noise function used to generate caves
float Chunk::perlinNoise3D(glm::vec3 uv) {
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            for(int dz = 0; dz <= 1; ++dz) {
                surfletSum += surflet3D(uv, glm::floor(uv) + glm::vec3(dx, dy, dz));
            }

        }
    }
    return surfletSum;
}

// given an x and z cord, sets all block types for all y values in that column
void Chunk::setBlock(int x, int z){
    float humidity = -perlinNoise(glm::vec2((x+0.f)/300.0, (z+0.f)/300.0))+0.5;
    float temperature = perlinNoise2(glm::vec2((x + 0.f)/300.0, (z + 0.f)/300.0))+0.5;

    // Calulate mountain heightfield
    float perlin = (perlinNoise(glm::vec2(x/64.0 ,z/64.0) ) + 0.5);
    float fbmNoise = fbm(perlin);
    float mountain = -508*fbmNoise + 203.2 ;
    mountain = std::max(std::min(
                     mountain,127.f),0.f); // mountain height
    mountain+=128;

    // Calculate grassland heightfield
    float worley = WorleyDist(glm::vec2(x/64.0 ,z/64.0));
    float grassland = -25*worley + 25;
    grassland = std::max(std::min(
                     grassland,40.f),0.f); // hill height
    grassland+=128;

    // Calculate desert heightfield
    float worley2 = WorleyDist(glm::vec2(x/64.0 ,z/62.0));
    float desert = -29*worley2 + 25;
    desert = std::max(std::min(
                     desert,10.f),0.f); // hill height
    desert+=140;

    // Calculate canion heightfield
    float perlinC = (perlinNoise(glm::vec2(x/300.0 ,z/300.0) ) + 0.5);
    float fbmNoiseC = fbm(perlinC, 0.1, 16, 100.f, 2.f);
    float canion = -508*fbmNoiseC + 203.2 ;
    canion = std::max(std::min(
                     canion,127.f),0.f); // canion height
    canion+=128;

    if (canion < 180.f) {
        canion -= 100.f;

        if (canion < 138.f) {// reduce water basin
            canion -= 10.f;
        }
    } else if (canion > 210) {
        canion = 210;
    }

    canion = std::max(std::min(
                     canion,255.f),1.f); // canion height

    // Mix heightfields
    int maxHeightGM;
    int maxHeightCD;
    float mixParamHum = glm::smoothstep(0.4f, 0.6f, humidity);
    float mixParamTmp = glm::smoothstep(0.4f, 0.6f, temperature);

    maxHeightGM = int(glm::mix(mountain, grassland, mixParamHum));
    maxHeightGM = std::max(std::min(
                     maxHeightGM,254),1); // interpolated value
    maxHeightCD = int(glm::mix(desert, canion, mixParamHum));
    maxHeightCD = std::max(std::min(
                     maxHeightCD,254),1); // interpolated value

    int maxHeight = int(glm::mix(maxHeightGM, maxHeightCD, mixParamTmp));
    maxHeight = std::max(std::min(
                     maxHeight,254),1); // interpolated value
    // Draw terrain
    if(mixParamHum <= 0.5 && mixParamTmp <= 0.5){ // Draw Mountains
        for(int i = 129; i <= maxHeight; i++){
            if(i == maxHeight && maxHeight >= 200){
                setBlockAt(x, i, z, SNOW); // top of mountain
            }else if (i < 150 && maxHeight < 150){
                setBlockAt(x, i, z, DIRT); // Plateau grass
            } else {
                setBlockAt(x, i, z, STONE); // set mountains stone
            }
        }
    }
    else if (mixParamHum > 0.5 && mixParamTmp <= 0.5){ // Draw Grasslan
        for(int i = 129; i <= maxHeight; i++){
            if(i == maxHeight){
                setBlockAt(x, i, z, GRASS); // top of hills
            }else{
                setBlockAt(x, i, z, DIRT); // set hills dirt
            }
        }
    } else if (mixParamHum <= 0.5 && mixParamTmp > 0.5) { // Draw Desert
        for(int i = 129; i <= maxHeight; i++){
            setBlockAt(x, i, z, SAND); // top of hills
        }
    } else if (mixParamHum > 0.5 && mixParamTmp > 0.5) { // Draw Canions
        for(int i = 129; i <= maxHeight; i++){
            if (i > 200) {
                setBlockAt(x, i, z, STONE); // Capstone
            } else {
                setBlockAt(x, i, z, MOSS_STONE); // Mossy body
            }
        }
    }

    // Add water
    for(int i = maxHeight; i < 138; i++){
        if (i == maxHeight) {
            setBlockAt(x, maxHeight, z, DIRT);
            continue;
        }
        setBlockAt(x, i, z, WATER); // water 128 - 138
    }


    //caves
    for(int i = 108; i <= 128; i++){
        float perlin = perlinNoise3D(glm::vec3(x/10.0,i/10.0,z/10.0));

        if(perlin > 0){
            setBlockAt(x, i, z, STONE);
        }else if (i < 113){ // should be 25 (just for testing)
            setBlockAt(x, i, z, LAVA);
        }else{
            setBlockAt(x, i, z, EMPTY);
        }
    }
    setBlockAt(x, 107, z, BEDROCK); // bottom layer is bedrock
}
