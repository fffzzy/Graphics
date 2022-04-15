#include "player.h"
#include <QString>

bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i) { // Iterate over the three axes
            if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if(axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if(interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }

    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

Player::Player(glm::vec3 pos, Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      mcr_camera(m_camera), isFlight(true), mcr_posPrev(0,0,0)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::toggleFlight() {
    isFlight = !isFlight;
}

void Player::processInputs(InputBundle &inputs) {
    // TODO: Update the Player's velocity and acceleration based on the
    // state of the inputs.
//    m_camera.RecomputeAttributes(inputs.mouseX, inputs.mouseY);

    if (isFlight) {
        if (inputs.wPressed) {
            m_acceleration = acceleration * m_forward;
        } else if (inputs.sPressed) {
            m_acceleration = -acceleration * m_forward;
        } else if (inputs.aPressed) {
            m_acceleration = -acceleration * m_right;
        } else if (inputs.dPressed) {
            m_acceleration = acceleration * m_right;
        } else if (inputs.qPressed) {
            m_acceleration = -acceleration * m_up;
        } else if (inputs.ePressed) {
            m_acceleration = acceleration * m_up;
        } else {
            m_acceleration = glm::vec3(0.f, 0.f, 0.f);
        }
    } else {
        BlockType underPlayer = mcr_terrain.getBlockAt(m_position.x, m_position.y - 0.5, m_position.z);
        if (inputs.wPressed) {
            glm::vec3 tempAcc = acceleration * m_forward;
            tempAcc.y = 0;
            m_acceleration = glm::normalize(tempAcc);
        } else if (inputs.sPressed) {
            glm::vec3 tempAcc = -acceleration * m_forward;
            tempAcc.y = 0;
            m_acceleration = glm::normalize(tempAcc);
        } else if (inputs.aPressed) {
            glm::vec3 tempAcc = -acceleration * m_right;
            tempAcc.y = 0;
            m_acceleration = glm::normalize(tempAcc);
        } else if (inputs.dPressed) {
            glm::vec3 tempAcc = acceleration * m_right;
            tempAcc.y = 0;
            m_acceleration = glm::normalize(tempAcc);
        } else {
            m_acceleration.x = 0;
            m_acceleration.z = 0;
        }

        // Apply vertical motion
        if (underPlayer == EMPTY) {
            m_acceleration.y = -g * m_up.y;
        } else {
            m_acceleration.y = 0;
            if (inputs.spacePressed) {
                m_velocity.y = jumpSpeed;
            } else {
                m_velocity.y = 0;
            }
        }
    }

}

void Player::computePhysics(float dT, Terrain &terrain) {
    // TODO: Update the Player's position based on its acceleration
    // and velocity, and also perform collision detection.

    // Apply friction
    m_velocity.x *= 1 - friction;
    m_velocity.z *= 1 - friction;

    // Apply acceleration
    m_velocity += m_acceleration * dT;

    // Clamp velocity
    m_velocity = glm::clamp(m_velocity, glm::vec3(-50.f, -100.f, -50.f), glm::vec3(50.f, 400.f, 50.f));
    glm::vec3 move = m_velocity * dT * 0.0003f;
    if (isFlight) {
        moveAlongVector(move);
    } else {
        moveWithCollisions(move);
    }
}

void Player::moveWithCollisions(glm::vec3 move) {
    float out_dist = -1.f;
    glm::ivec3 out_blockHit = glm::ivec3();

    auto p1 = glm::vec3(m_position.x + 0.5, m_position.y, m_position.z - 0.5);
    auto p2 = glm::vec3(m_position.x - 0.5, m_position.y, m_position.z - 0.5);
    auto p3 = glm::vec3(m_position.x + 0.5, m_position.y, m_position.z + 0.5);
    auto p4 = glm::vec3(m_position.x - 0.5, m_position.y, m_position.z + 0.5);
    auto p5 = glm::vec3(m_position.x + 0.5, m_position.y + 1, m_position.z - 0.5);
    auto p6 = glm::vec3(m_position.x - 0.5, m_position.y + 1, m_position.z - 0.5);
    auto p7 = glm::vec3(m_position.x + 0.5, m_position.y + 1, m_position.z + 0.5);
    auto p8 = glm::vec3(m_position.x - 0.5, m_position.y + 1, m_position.z + 0.5);
    auto p9 = glm::vec3(m_position.x + 0.5, m_position.y + 2, m_position.z - 0.5);
    auto p10 = glm::vec3(m_position.x - 0.5, m_position.y + 2, m_position.z - 0.5);
    auto p11 = glm::vec3(m_position.x + 0.5, m_position.y + 2, m_position.z + 0.5);
    auto p12 = glm::vec3(m_position.x - 0.5, m_position.y + 2, m_position.z + 0.5);
    std::vector<glm::vec3> box = {p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12};
    for (int j = 0; j < 12; j++) {
        for (int i = 0; i < 3; i++) {
            //(0,1,0), (1, 0, 0), (0, 0, 1)
            glm::vec3 detectDir = glm::vec3();
            detectDir[i] = move[i];
            bool isBlock = gridMarch(box[j], detectDir, mcr_terrain, &out_dist, &out_blockHit);
            if (isBlock) {
                //super robust block detector
                if (out_dist > 0.001f) {
                    move[i] = glm::sign(move[i]) * (glm::min(glm::abs(move[i]), out_dist) - 0.0001f);
                } else {
                    move[i] = 0;
                }
            }
        }
    }
    moveAlongVector(move);
}

void Player::addBlock() {
    float out_dist = -1.f;
    glm::ivec3 out_blockHit = glm::ivec3();
    bool isBlock = gridMarch(m_camera.mcr_position, m_forward, mcr_terrain, &out_dist, &out_blockHit);
    if (!isBlock) {
        out_blockHit = m_camera.mcr_position + 3.f * glm::normalize(this->m_forward);
        Chunk* c = mcr_terrain.getChunkAt(out_blockHit.x, out_blockHit.z).get();
        glm::vec2 chunkOrigin = glm::vec2(floor(out_blockHit.x / 16.f) * 16, floor(out_blockHit.z / 16.f) * 16);

        if (c->getBlockAt(glm::vec3(static_cast<unsigned int>(out_blockHit.x - chunkOrigin.x),
                          static_cast<unsigned int>(out_blockHit.y),
                          static_cast<unsigned int>(out_blockHit.z - chunkOrigin.y))) == EMPTY) {
            c->setBlockAt(static_cast<unsigned int>(out_blockHit.x - chunkOrigin.x),
                                 static_cast<unsigned int>(out_blockHit.y),
                                 static_cast<unsigned int>(out_blockHit.z - chunkOrigin.y), STONE);
        }
    }
}

void Player::removeBlock() {
    float out_dist = -1.f;
    glm::ivec3 out_blockHit = glm::ivec3();
    bool isBlock = gridMarch(m_camera.mcr_position, 3.f * m_forward, mcr_terrain, &out_dist, &out_blockHit);
    if (isBlock) {
        mcr_terrain.setBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z, EMPTY);
    }

}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}
