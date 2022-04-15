#include "chunk.h"


Chunk::Chunk(OpenGLContext* mp_context) : Drawable(mp_context), m_blocks(),
    m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}},
    m_chunkVBOData(this), hasVBOdata(false)
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
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
}

// Exists to simplify calls by allowing a vector argument
BlockType Chunk::getBlockAt(glm::vec3 pos) const {
    return getBlockAt(static_cast<unsigned int>(pos.x), static_cast<unsigned int>(pos.y), static_cast<unsigned int>(pos.z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at((x % 16) + 16 * (y % 256) + 16 * 256 * (z % 16)) = t;
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

                        // If the neighbor is empty, add vertices to collection
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

    this->bufferVBOdata(O_interleavedVector, O_idx, T_interleavedVector, T_idx);

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

void Chunk::generateChunk(int PosX, int PosZ){
    // Populate blocks
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 16; j++){
            setBlock(PosX + i, PosZ + j);
        }
    }
}

void Chunk::destroyVBOdata() {
    Drawable::destroyVBOdata();
    this->hasVBOdata = false;
}

glm::vec2 Chunk::random2( glm::vec2 p ) {
    return glm::fract(glm::sin(glm::vec2(glm::dot(p, glm::vec2(127.1, 311.7)),
                 glm::dot(p, glm::vec2(269.5,183.3))))
                 * (float)43758.5453);
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

float Chunk::fbm(float x) {
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

float Chunk::WorleyDist(glm::vec2 uv) {
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

void Chunk::setBlock(int x, int z){
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
