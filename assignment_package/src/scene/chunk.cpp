#include "chunk.h"


Chunk::Chunk(OpenGLContext* mp_context) : Drawable(mp_context), m_blocks(),
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
