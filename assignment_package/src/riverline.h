#pragma once

#include <glm_includes.h>

class RiverLine
{
public:
    RiverLine(glm::vec2 p1, glm::vec2 p2, float width);
    glm::vec2 p1;
    glm::vec2 p2;
    float width;
};
