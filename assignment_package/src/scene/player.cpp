#include "player.h"
#include <QString>
#include <glm/gtx/vector_angle.hpp>

#define MOUSE_SENSITIVITY 0.5f
#define ACCELERATION_CONST 2.0f
#define VELOCITY_CONST 4.0f
#define ACCELERATION_DECAY 0.75f
#define VELOCITY_DECAY 0.85f
#define JUMP_CONST 25.0f
#define GRAVITY_CONST -3.0f
#define REACH_DIST 3.0f
#define COLLISION_THRESHOLD 0.00001f
#define BOUNDING_BOX_WIDTH 0.48f
#define BOUNDING_BOX_HEIGHT 1.8f
#define FLOAT_EPSILON 0.0001f
#define FLYING_MUL 2.5f

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
    m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
    flight(true), inAir(true), phi(0),
    inWater(0), inLava(0), mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::processInputs(InputBundle &inputs) {
    // toggle flight
    if (inputs.fPressed) {
        flight = !flight;
        if (!flight) inAir = true;
        inputs.fPressed = false;
    }

    // change look direction
    rotateOnUpGlobal(-1.0f * inputs.mouseX * MOUSE_SENSITIVITY);
    rotateOnRightLocal(-1.0f * inputs.mouseY * MOUSE_SENSITIVITY);

    // directional accelerations
    glm::vec3 accelDir = glm::vec3();
    if (inputs.wPressed) accelDir += m_forward;
    if (inputs.sPressed) accelDir -= m_forward;
    if (inputs.dPressed) accelDir += m_right;
    if (inputs.aPressed) accelDir -= m_right;

    if (flight) {
        // flying mode inputs
        if (inputs.ePressed) accelDir += m_up;
        if (inputs.qPressed) accelDir -= m_up;

    } else {

        if (inputs.spacePressed && (this->getBodyInLava() || this->getBodyInWater())) {
            m_velocity.y += (JUMP_CONST * 0.1);
            accelDir.y = 0;
        } else {
            // not flying inputs
            if (inputs.spacePressed && !inAir) {
                m_velocity.y += JUMP_CONST;
                inAir = true;
            }

            accelDir.y = 0;
        }


    }

    // renormalize acceleration direction
    if (glm::length(accelDir) > 1) {
        accelDir = glm::normalize(accelDir);
    }

    // add new acceleration
    m_acceleration += ACCELERATION_CONST * accelDir;
}

// grid marching from the slides
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
        if (!terrain.hasChunkAt(currCell.x, currCell.z)) {
            *out_dist = glm::min(maxLen, curr_t);
            return false;
        }
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY && cellType != WATER && cellType != LAVA) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

// array for points to check in the bounding box of the player, constant for each call
const std::array<glm::vec3,12> boundingBox = {glm::vec3(BOUNDING_BOX_WIDTH,0,BOUNDING_BOX_WIDTH),
                                               glm::vec3(-1*BOUNDING_BOX_WIDTH,0,BOUNDING_BOX_WIDTH),
                                               glm::vec3(-1*BOUNDING_BOX_WIDTH,0,-1*BOUNDING_BOX_WIDTH),
                                               glm::vec3(BOUNDING_BOX_WIDTH,0,-1*BOUNDING_BOX_WIDTH),
                                               glm::vec3(BOUNDING_BOX_WIDTH,0.5*BOUNDING_BOX_HEIGHT,BOUNDING_BOX_WIDTH),
                                               glm::vec3(-1*BOUNDING_BOX_WIDTH,0.5*BOUNDING_BOX_HEIGHT,BOUNDING_BOX_WIDTH),
                                               glm::vec3(-1*BOUNDING_BOX_WIDTH,0.5*BOUNDING_BOX_HEIGHT,-1*BOUNDING_BOX_WIDTH),
                                               glm::vec3(BOUNDING_BOX_WIDTH,0.5*BOUNDING_BOX_HEIGHT,-1*BOUNDING_BOX_WIDTH),
                                               glm::vec3(BOUNDING_BOX_WIDTH,BOUNDING_BOX_HEIGHT,BOUNDING_BOX_WIDTH),
                                               glm::vec3(-1*BOUNDING_BOX_WIDTH,BOUNDING_BOX_HEIGHT,BOUNDING_BOX_WIDTH),
                                               glm::vec3(-1*BOUNDING_BOX_WIDTH,BOUNDING_BOX_HEIGHT,-1*BOUNDING_BOX_WIDTH),
                                               glm::vec3(BOUNDING_BOX_WIDTH,BOUNDING_BOX_HEIGHT,-1*BOUNDING_BOX_WIDTH)};

// collide the player with the terrian using grid marching technique
void Player::collideAndMove(const Terrain& terrain, float dt) {

    // loop through axes
    std::array<glm::vec3,3> axisDirections = {glm::vec3(1,0,0), glm::vec3(0,1,0), glm::vec3(0,0,1)};
    for (int i=0;i<3;i++) {
        float moveDist = glm::abs(m_velocity[i] * dt);
        float velSign = glm::sign(m_velocity[i]);

        // loop through points in the bounding box
        for (const glm::vec3& point : boundingBox) {
            float collisionDist = moveDist;
            glm::ivec3 block;

            gridMarch(point + m_position, m_velocity[i] * dt * axisDirections[i], terrain,&collisionDist, &block);

            collisionDist = collisionDist - COLLISION_THRESHOLD;
            moveDist = glm::min(moveDist,collisionDist);
        }

        // in air checks
        if (i == 1) {
            if (moveDist < 0.00001f && inAir) inAir = false;
            if (moveDist > 0.00001f && velSign < 0 && !inAir) inAir = true;
        }

        moveAlongVector(moveDist * velSign * axisDirections[i]);
    }


}

void Player::computePhysics(float dT, const Terrain &terrain) {

    if (terrain.hasChunkAt(m_position.x, m_position.z)) {
        //check for in water/lava
        if (terrain.getBlockAt(this->m_position + glm::vec3(0, 1.5f, 0)) == BlockType::WATER) {
            this->inWater = 2;
            this->inLava = 0;
        } else if (terrain.getBlockAt(this->m_position + glm::vec3(0, 1.5f, 0)) == BlockType::LAVA) {
            this->inWater = 0;
            this->inLava = 2;
        } else {
            if (terrain.getBlockAt(this->m_position) == BlockType::WATER) {
                this->inWater = 1;
                this->inLava = 0;
            } else if (terrain.getBlockAt(this->m_position) == BlockType::LAVA) {
                this->inWater = 0;
                this->inLava = 1;
            } else {
                this->inWater = 0;
                this->inLava = 0;
            }
        }
    }

    float mult = (this->getBodyInLava() || this->getBodyInWater()) ? 0.66 : 1;

    // calculate gravity
    if (!flight) m_acceleration.y += (GRAVITY_CONST * mult);

    // calculate velocity
    m_velocity += VELOCITY_CONST * m_acceleration * dT;
    m_velocity *= mult;

    // move and or collide
    if (flight) {
        moveAlongVector(m_velocity * dT * FLYING_MUL);
    } else {
        collideAndMove(terrain, dT);
    }

    // decay acceleration and velocity
    m_acceleration *= ACCELERATION_DECAY;
    m_velocity *= VELOCITY_DECAY;
}

void Player::breakBlock(Terrain& terrain) const {
    float dist;
    glm::ivec3 block;
    if (gridMarch(mcr_camera.mcr_position, REACH_DIST * m_forward, terrain, &dist, &block) &&
        terrain.getBlockAt(block.x, block.y, block.z) != BlockType::BEDROCK) {
        terrain.setBlockAt(block.x,block.y,block.z,EMPTY);

        Chunk* chunkWithBlock = terrain.getChunkAt(block.x,block.z).get();
        std::array<Chunk*,4> neighborChunkBlocks = {terrain.getChunkAt(block.x - 1,block.z).get(),terrain.getChunkAt(block.x + 1,block.z).get(),terrain.getChunkAt(block.x,block.z - 1).get(),terrain.getChunkAt(block.x,block.z + 1).get()};

        for (Chunk* c : neighborChunkBlocks) {
            if (c != chunkWithBlock) {
                c->resetVBOData();
            }
        }
        chunkWithBlock->resetVBOData();
    }
}

void Player::placeBlock(Terrain& terrain, BlockType placingBlock) const {
    float dist;
    glm::ivec3 block;
    if (gridMarch(m_camera.mcr_position, REACH_DIST * m_forward, terrain, &dist, &block)) {
        glm::ivec3 placedBlock = block;

        // get the position where the ray intersected
        glm::vec3 rayIntersection = m_camera.mcr_position + m_forward * dist;
        glm::vec3 positionOnBlock = rayIntersection - glm::vec3(block);

        // place block according to face intersected
        bool xInBounds = (positionOnBlock.x > 0 && positionOnBlock.x < 1);
        bool yInBounds = (positionOnBlock.y > 0 && positionOnBlock.y < 1);
        bool zInBounds = (positionOnBlock.z > 0 && positionOnBlock.z < 1);
        if (xInBounds && yInBounds && positionOnBlock.z <= 0 + FLOAT_EPSILON) placedBlock.z--;
        if (xInBounds && yInBounds && positionOnBlock.z >= 1 - FLOAT_EPSILON) placedBlock.z++;
        if (xInBounds && positionOnBlock.y <= 0 + FLOAT_EPSILON && zInBounds) placedBlock.y--;
        if (xInBounds && positionOnBlock.y >= 1 - FLOAT_EPSILON && zInBounds) placedBlock.y++;
        if (positionOnBlock.x <= 0 + FLOAT_EPSILON && yInBounds && zInBounds) placedBlock.x--;
        if (positionOnBlock.x >= 1 - FLOAT_EPSILON && yInBounds && zInBounds) placedBlock.x++;

        // check if block would be placed in the player, return if yes
        glm::vec3 placedBlockBottomCenter = glm::vec3(placedBlock) + glm::vec3(0.5,0,0.5);
        if (glm::length(placedBlockBottomCenter - m_position) < 1 ||
            glm::length(placedBlockBottomCenter - m_camera.mcr_position) < 1) return;

        // else place
        terrain.setBlockAt(placedBlock.x,placedBlock.y,placedBlock.z,placingBlock);
        terrain.getChunkAt(placedBlock.x,placedBlock.z)->resetVBOData();
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
    // for gimbal locking
    float oldPhi = phi;
    phi = (glm::abs(phi + degrees) > 89.99) ? glm::sign(phi) * 89.9 : phi + degrees;
    degrees = phi - oldPhi;

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

bool Player::getCamInWater() const {
    return this->inWater == 2;
}

bool Player::getCamInLava() const {
    return this->inLava == 2;
}

bool Player::getBodyInWater() const {
    return this->inWater > 0;
}

bool Player::getBodyInLava() const {
    return this->inLava > 0;
}
