#include "chunk.h"


Chunk::Chunk(OpenGLContext* mp_context, int Xoffset, int Zoffset) : Drawable(mp_context), m_Xoffset(Xoffset), m_Zoffset(Zoffset), m_blocks(), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}}
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    // Check for limits
    if ((int) x < 0) {
        // Check for no neighbor
        if (m_neighbors.at(XNEG) == nullptr) {
            return EMPTY;
        }
        return m_neighbors.at(XNEG)->getBlockAt(16 + x, y, z);
    } else if ((int) x >= 16) {
        // Check for no neighbor
        if (m_neighbors.at(XPOS) == nullptr) {
            return EMPTY;
        }
        return m_neighbors.at(XPOS)->getBlockAt(x - 16, y, z);
    } else if ((int) y < 0) {
        // Check for no neighbor
        if (m_neighbors.at(YNEG) == nullptr) {
            return EMPTY;
        }
        return m_neighbors.at(YNEG)->getBlockAt(x, 256 + y, z);
    } else if ((int) y >= 256) {
        // Check for no neighbor
        if (m_neighbors.at(YPOS) == nullptr) {
            return EMPTY;
        }
        return m_neighbors.at(YPOS)->getBlockAt(x, y - 256, z);
    } else if ((int) z < 0) {
        // Check for no neighbor
        if (m_neighbors.at(ZNEG) == nullptr) {
            return EMPTY;
        }
        return m_neighbors.at(ZNEG)->getBlockAt(x, y, 16 + z);
    } else if ((int) z >= 16) {
        // Check for no neighbor
        if (m_neighbors.at(ZPOS) == nullptr) {
            return EMPTY;
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

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

void Chunk::createVBOdata() {
    // Create stores for all the square faces to be drawn
    std::vector<glm::vec4> pos = std::vector<glm::vec4>();
    std::vector<glm::vec4> nor = std::vector<glm::vec4>();
    std::vector<glm::vec4> col = std::vector<glm::vec4>();
    std::vector<int> idx = std::vector<int>();

    int indexOffset = 0;

    // Iterate through all the blocks
    for (int z = 0; z < 16; z++) {
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 16; x++) {
                BlockType btAtCurrPos = getBlockAt(x, y, z);

                if (btAtCurrPos != EMPTY) {
                    glm::vec3 currPos = glm::vec3(x, y, z);
                    glm::vec3 currWorldPos = glm::vec3(x+m_Xoffset, y, z+m_Zoffset);

                    // Look at all neighbors and add appropriate faces
                    for (BlockFace neighborFace : adjacentFaces) {
                        BlockType neighborType = getBlockAt(neighborFace.directionVec + currPos);

                        // If the neighbor is empty, add vertices to collection
                        if (neighborType == EMPTY) {
                            for (VertexData VD : neighborFace.vertices) {
                                pos.push_back(glm::vec4(currWorldPos, 0.f) + VD.pos);
                                nor.push_back(glm::vec4(neighborFace.directionVec, 0.f));

                                switch(btAtCurrPos) {
                                    case GRASS:
                                        col.push_back(glm::vec4(95.f, 159.f, 53.f, 0.f) / 255.f);
                                        break;
                                    case DIRT:
                                        col.push_back(glm::vec4(121.f, 85.f, 58.f, 0.f) / 255.f);
                                        break;
                                    case STONE:
                                        col.push_back(glm::vec4(0.5f, 0.5f, 0.5f, 0.f));
                                        break;
                                    case SNOW:
                                        col.push_back(glm::vec4(1.f, 1.f, 1.f, 0.f));
                                        break;
                                    case WATER:
                                        col.push_back(glm::vec4(0.f, 0.f, 0.75f, 0.f));
                                        break;
                                    default:
                                        // Other block types are not yet handled, so we default to debug purple
                                        col.push_back(glm::vec4(1.f, 0.f, 1.f, 0.f));
                                        break;
                                }

                                // Push indices
                                idx.push_back(0 + indexOffset);
                                idx.push_back(1 + indexOffset);
                                idx.push_back(2 + indexOffset);
                                idx.push_back(0 + indexOffset);
                                idx.push_back(2 + indexOffset);
                                idx.push_back(3 + indexOffset);
                                indexOffset += 4;
                            }
                        }
                    }
                }
            }
        }
    }

    std::vector<glm::vec4> interleavedVector = std::vector<glm::vec4>();

    for (int i = 0; i < pos.size(); i++) {
        interleavedVector.push_back(pos[i]);
        interleavedVector.push_back(nor[i]);
        interleavedVector.push_back(col[i]);
    }

    this->bufferVBOdata(interleavedVector, idx);
}

void Chunk::bufferVBOdata(std::vector<glm::vec4> interleavedData, std::vector<int> indices) {
    this->m_count = indices.size();

    generatePos();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    mp_context->glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(glm::vec4), interleavedData.data(), GL_STATIC_DRAW);

    generateNor();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufNor);
    mp_context->glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(glm::vec4), interleavedData.data(), GL_STATIC_DRAW);

    generateCol();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufCol);
    mp_context->glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(glm::vec4), interleavedData.data(), GL_STATIC_DRAW);

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
}
