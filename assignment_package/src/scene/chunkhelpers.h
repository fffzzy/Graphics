#ifndef CHUNKHELPERS_H
#define CHUNKHELPERS_H

#include "glm/glm.hpp"

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, SNOW, UNDETERMINED, LAVA, BEDROCK
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// A struct to store vertex data
struct VertexData {
    // The vertex position
    glm::vec4 pos;

    // The vertex uv coordinate
    glm::vec2 uv;

    VertexData(glm::vec4 p, glm::vec2 u) : pos(p), uv(u)
    {}
};

// A struct to describe different block faces
struct BlockFace {
    Direction direction;
    glm::vec3 directionVec;
    std::array<VertexData, 4> vertices;
    BlockFace(Direction dir, glm::vec3 dirV, const VertexData &a, const VertexData &b, const VertexData &c, const VertexData &d) :
        direction(dir), directionVec(dirV), vertices{a, b, c, d}
    {}
};

#define BLK_UV 1/16.f

const static std::array<BlockFace, 6> adjacentFaces {
            // +X
            BlockFace(XPOS, glm::vec3(1, 0, 0), VertexData(glm::vec4(1, 0, 1, 1), glm::vec2(0, 0)),
                                                VertexData(glm::vec4(1, 0, 0, 1), glm::vec2(BLK_UV, 0)),
                                                VertexData(glm::vec4(1, 1, 0, 1), glm::vec2(BLK_UV, BLK_UV)),
                                                VertexData(glm::vec4(1, 1, 1, 1), glm::vec2(0, BLK_UV))),
            // -X
            BlockFace(XNEG, glm::vec3(-1, 0, 0), VertexData(glm::vec4(0, 0, 0, 1), glm::vec2(0, 0)),
                                                 VertexData(glm::vec4(0, 0, 1, 1), glm::vec2(BLK_UV, 0)),
                                                 VertexData(glm::vec4(0, 1, 1, 1), glm::vec2(BLK_UV, BLK_UV)),
                                                 VertexData(glm::vec4(0, 1, 0, 1), glm::vec2(0, BLK_UV))),
            // +Y
            BlockFace(YPOS, glm::vec3(0, 1, 0),  VertexData(glm::vec4(0, 1, 1, 1), glm::vec2(0, 0)),
                                                 VertexData(glm::vec4(1, 1, 1, 1), glm::vec2(BLK_UV, 0)),
                                                 VertexData(glm::vec4(1, 1, 0, 1), glm::vec2(BLK_UV, BLK_UV)),
                                                 VertexData(glm::vec4(0, 1, 0, 1), glm::vec2(0, BLK_UV))),
            // -Y
            BlockFace(YNEG, glm::vec3(0, -1, 0), VertexData(glm::vec4(0, 0, 0, 1), glm::vec2(0, 0)),
                                                 VertexData(glm::vec4(1, 0, 0, 1), glm::vec2(BLK_UV, 0)),
                                                 VertexData(glm::vec4(1, 0, 1, 1), glm::vec2(BLK_UV, BLK_UV)),
                                                 VertexData(glm::vec4(0, 0, 1, 1), glm::vec2(0, BLK_UV))),
            // +Z
            BlockFace(ZPOS, glm::vec3(0, 0, 1), VertexData(glm::vec4(0, 0, 1, 1), glm::vec2(0, 0)),
                                                VertexData(glm::vec4(1, 0, 1, 1), glm::vec2(BLK_UV, 0)),
                                                VertexData(glm::vec4(1, 1, 1, 1), glm::vec2(BLK_UV, BLK_UV)),
                                                VertexData(glm::vec4(0, 1, 1, 1), glm::vec2(0, BLK_UV))),
            // -Z
            BlockFace(ZNEG, glm::vec3(0, 0, -1), VertexData(glm::vec4(1, 0, 0, 1), glm::vec2(0, 0)),
                                                 VertexData(glm::vec4(0, 0, 0, 1), glm::vec2(BLK_UV, 0)),
                                                 VertexData(glm::vec4(0, 1, 0, 1), glm::vec2(BLK_UV, BLK_UV)),
                                                 VertexData(glm::vec4(1, 1, 0, 1), glm::vec2(0, BLK_UV)))
};

#endif // CHUNKHELPERS_H
