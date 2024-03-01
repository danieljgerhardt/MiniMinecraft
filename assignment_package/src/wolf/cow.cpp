#include "cow.h"

#define COLLISION_THRESHOLD 0.00001f
#define ACCELERATION_CONST 2.0f
#define VELOCITY_CONST 4.0f
#define ACCELERATION_DECAY 0.75f
#define VELOCITY_DECAY 0.85f
#define JUMP_CONST 25.0f
#define GRAVITY_CONST -3.0f
#define MIN_DIST 10.f

Cow::Cow(OpenGLContext *context, std::vector<glm::vec3>& vertices,
           std::vector<glm::vec3>& normals, std::vector<std::vector<int>>& faces, glm::vec3 pos, const Terrain &t)
    : EntityDrawable(context), Entity(pos), hasBeenSkinned(false), terrain(t), updatePosCounter(0), currTarget(glm::vec3()) {
    std::unordered_map<std::pair<Vertex*, Vertex*>, HalfEdge*, pair_hash> twinEdgeMap;

    for (int i = 0; i < (int)vertices.size(); i++) {
        uPtr<Vertex> vertex = mkU<Vertex>();
        vertex->pos = vertices.at(i);
        vertex->nor = normals.at(i);
        this->addVertex(std::move(vertex));
    }

    //hes and faces
    for (const std::vector<int>& faceIndices : faces) {
        int numEdges = faceIndices.size();
        std::vector<HalfEdge*> faceHalfEdges;

        uPtr<Face> face = mkU<Face>();

        for (int i = 0; i < numEdges; ++i) {
            int startVertexIdx = faceIndices[i];
            int endVertexIdx = faceIndices[(i + 1) % numEdges];

            //make halfedge, set its vertex, set the vertex's he, add the edge to the face
            uPtr<HalfEdge> he = mkU<HalfEdge>();
            he->setVertex(this->vertices[endVertexIdx].get());
            this->vertices[endVertexIdx]->he = he.get();
            faceHalfEdges.push_back(he.get());

            //use hash map to set syms
            int minIdx = std::min(startVertexIdx, endVertexIdx);
            int maxIdx = std::max(startVertexIdx, endVertexIdx);
            std::pair<Vertex*, Vertex*> key;

            key.first = this->vertices[minIdx].get();
            key.second = this->vertices[maxIdx].get();
            //check if edge is in map
            if (twinEdgeMap.find(key) == twinEdgeMap.end()) {
                //edge not in map
                twinEdgeMap.insert({key, he.get()});
            } else {
                //edge in map
                he->setSym(twinEdgeMap.at(key));
            }

            face->setEdge(he.get());

            //add half edge to mesh
            this->addHE(std::move(he));
        }

        //set nexts of half edges
        for (int i = 0; i < numEdges; ++i) {
            int nextIdx = (i + 1) % numEdges;
            faceHalfEdges[i]->setNext(faceHalfEdges[nextIdx]);
        }

        float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float g = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        float b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        face->color = glm::vec3(r, g, b);
        this->addFace(std::move(face));
    }
}

Cow::~Cow() {}

int Cow::calcTotalIndices() {
    int num = 0;

    for (uPtr<Face>& fPtr : this->faces) {
        HalfEdge* faceEdge = fPtr->getHE();
        HalfEdge* next = faceEdge->next;
        while(next != faceEdge){
            next = next->next;
            num++;
        }
    }
    return 3 * (num - faces.size());
}

void Cow::create() {
    int elems = hes.size(), idx_count = calcTotalIndices();
    std::vector<glm::vec4> col(elems);
    std::vector<glm::vec4> pos(elems);
    std::vector<glm::vec4> nor(elems);
    std::vector<GLuint> idx(idx_count);

    std::vector<glm::ivec2> ids(elems);
    std::vector<glm::vec2> weights(elems);

    //index is used for the first part of the loop, setting up pos col and nor
    int index = 0;
    //baseIdx and vertex_count are used for the second part of the loop, setting up idx
    int baseIdx = 0, vertex_count = 0;
    for (uPtr<Face>& facePtr : this->faces) {
        HalfEdge* faceHE = facePtr->getHE();

        Vertex *v1 = faceHE->v, *v2 = faceHE->next->v, *v3 = faceHE->next->next->v;
        glm::vec3 normal = glm::normalize(glm::cross(v1->pos - v3->pos, v2->pos - v3->pos));

        HalfEdge* loopHE = faceHE;
        do {
            pos[index] = glm::vec4(loopHE->v->pos, 1);
            col[index] = glm::vec4(facePtr->color, 1);
            nor[index] = glm::vec4(normal, 1);

            if (this->skinned()) {
                std::vector<std::pair<Joint*, float>>& currInfluences = loopHE->v->getInfluences();
                glm::ivec2 newId = glm::ivec2();
                glm::vec2 newWeight = glm::vec2();
                newId[0] = currInfluences[0].first->getId() - 1;
                newId[1] = currInfluences[1].first->getId() - 1;
                newWeight[0] = currInfluences[0].second;
                newWeight[1] = currInfluences[1].second;
                ids[index] = newId;
                weights[index] = newWeight;
            }

            loopHE = loopHE->next;
            index++;
        } while (loopHE != faceHE);

        //loop part 2! time for idx
        //step 1 - count da verts
        int numVerts = 0;
        loopHE = faceHE;
        do {
            numVerts++;
            loopHE = loopHE->getNext();
        } while (loopHE != faceHE);

        //step 2 - use vert count to place verts in the right spot in idx
        for (int j = 2; j < numVerts; j++) {
            int startIdx = baseIdx * 3;
            idx[startIdx] = vertex_count;
            idx[startIdx + 1] = vertex_count + j - 1;
            idx[startIdx + 2] = vertex_count + j;
            baseIdx++;
        }
        //accumulate the total vertex count so far to start in the right place next time
        vertex_count += numVerts;
    }

    count = calcTotalIndices();

    if (this->skinned()) {
        generateIds();
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufIds);
        mp_context->glBufferData(GL_ARRAY_BUFFER, ids.size() * sizeof(glm::ivec2), ids.data(), GL_STATIC_DRAW);

        generateWeights();
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufWeights);
        mp_context->glBufferData(GL_ARRAY_BUFFER, weights.size() * sizeof(glm::vec2), weights.data(), GL_STATIC_DRAW);
    }

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    generatePos();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufPos);
    mp_context->glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(glm::vec4), pos.data(), GL_STATIC_DRAW);

    generateNor();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufNor);
    mp_context->glBufferData(GL_ARRAY_BUFFER, nor.size() * sizeof(glm::vec4), nor.data(), GL_STATIC_DRAW);

    generateCol();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufCol);
    mp_context->glBufferData(GL_ARRAY_BUFFER, col.size() * sizeof(glm::vec4), col.data(), GL_STATIC_DRAW);
}

GLenum Cow::drawMode() {
    return GL_TRIANGLES;
}

void Cow::addFace(uPtr<Face> fPtr) {
    this->faces.push_back(std::move(fPtr));
}

void Cow::addVertex(uPtr<Vertex> vPtr) {
    this->vertices.push_back(std::move(vPtr));
}

void Cow::addHE(uPtr<HalfEdge> hePtr) {
    this->hes.push_back(std::move(hePtr));
}

bool Cow::hasVertices() const {
    return !this->vertices.empty();
}

std::vector<uPtr<Vertex>>& Cow::getVertices() {
    return this->vertices;
}

std::vector<uPtr<Face>>& Cow::getFaces() {
    return this->faces;
}

std::vector<uPtr<HalfEdge>>& Cow::getHes() {
    return this->hes;
}

void Cow::setRootJoint(uPtr<Joint> newRoot) {
    this->joint = std::move(newRoot);
}

void Cow::findVertJointsRec(Vertex* v, Joint* curr, std::array<Joint*, 2>& jointArr, std::array<float, 2>& dists) {
    curr->updateBindMat();

    float currDist = glm::distance(glm::vec4(v->getPos(), 1), curr->getOverallTransformation() * glm::vec4(0,0,0,1));
    if (currDist < std::max(dists[0], dists[1])) {
        if (dists[0] > dists[1]) {
            dists[0] = currDist;
            jointArr[0] = curr;
        } else {
            dists[1] = currDist;
            jointArr[1] = curr;
        }
    }
    for (uPtr<Joint>& child : curr->getChildren()) {
        findVertJointsRec(v, child.get(), jointArr, dists);
    }
}

void Cow::skinCow() {
    for (int i = 0; i < (int) vertices.size();i++) {
        std::array<Joint*, 2> joints;
        std::array<float, 2> dists = {FLT_MAX, FLT_MAX};

        findVertJointsRec(vertices[i].get(), this->joint.get(), joints, dists);

        float total = dists[0] + dists[1];
        dists[0] /= total;
        dists[1] /= total;

        vertices[i]->addInfluence(joints[0], dists[0]);
        vertices[i]->addInfluence(joints[1], dists[1]);
    }
    this->hasBeenSkinned = true;
}

bool Cow::hasJoints() const {
    return !(this->joint == nullptr);
}

uPtr<Joint>& Cow::getRoot() {
    return this->joint;
}

bool Cow::skinned() const {
    return this->hasBeenSkinned;
}

void Cow::tick(float dT, qint64 currentTime) {
    this->updatePosCounter++;
    this->genInputs(dT);
    this->computePhysics(dT, terrain);
    if (this->isMoving()) {
        this->animateLegs((float)(currentTime % 10000));
    }
}

void Cow::tick(float dT, InputBundle& input) {}

// grid marching from the slides
bool gridMarchW(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
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
//bottom left = 3, -9, -10
//top right = -3, 7, 17
const std::array<glm::vec3, 8> boundingBox = {glm::vec3(.15, -.6, -.5),
                                              glm::vec3(.15, .25, -.5),
                                              glm::vec3(.15, -.6, .85),
                                              glm::vec3(.15, .25, .85),
                                              glm::vec3(-.15, -.6, -.5),
                                              glm::vec3(-.15, .25, -.5),
                                              glm::vec3(-.15, -.6, .85),
                                              glm::vec3(-.15, .25, .85)};

// collide the player with the terrian using grid marching technique
void Cow::collideAndMove(const Terrain& terrain, float dt) {

    // loop through axes
    std::array<glm::vec3,3> axisDirections = {glm::vec3(1,0,0), glm::vec3(0,1,0), glm::vec3(0,0,1)};
    for (int i = 0; i < 3; i++) {
        float moveDist = glm::abs(m_velocity[i] * dt);
        float velSign = glm::sign(m_velocity[i]);

        // loop through points in the bounding box
        for (const glm::vec3& point : boundingBox) {
            float collisionDist = moveDist;
            glm::ivec3 block;

            gridMarchW(point + m_position, m_velocity[i] * dt * axisDirections[i], terrain, &collisionDist, &block);

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

void Cow::computePhysics(float dT, const Terrain &terrain) {

    // calculate gravity
    m_acceleration.y += GRAVITY_CONST;

    // calculate velocity
    m_velocity += VELOCITY_CONST * m_acceleration * dT;

    // move and collide
    collideAndMove(terrain, dT);

    // decay acceleration and velocity
    m_acceleration *= ACCELERATION_DECAY;
    m_velocity *= VELOCITY_DECAY;
}

int getRandomIntInRange(int min, int max) {
    // Seed the random number generator
    std::random_device rd;
    std::mt19937 gen(rd());

    // Define the distribution for the desired range
    std::uniform_int_distribution<int> distribution(min, max);

    // Generate and return a random integer
    return distribution(gen);
}

void Cow::genInputs(float dT) {
    glm::vec3 accelDir = glm::vec3();

    if (updatePosCounter % 1000 == 0) {
        currTarget = glm::vec3(playerPos.x + getRandomIntInRange(-20, 20),
                               playerPos.y,
                               playerPos.z + getRandomIntInRange(-20, 20));
    }

    glm::vec3 direction = glm::normalize(currTarget - m_position);

    // Calculate the new position at the specified distance and same angle
    glm::vec3 targetPos = m_position + (glm::length(currTarget - m_position) - MIN_DIST) * direction;

    glm::vec3 targetVec = targetPos - m_position;

    accelDir += glm::vec3(targetVec.x, 0, targetVec.z);

    glm::ivec3 blockPos;
    float collisionDist;
    glm::vec3 rayDir = targetVec * dT;
    //jump check 1: if you need to go up or if you are stuck.
    //jump check 2: there is a block to jump on in your direction of movement.
    if ((rayDir.y > 0 || glm::length(m_velocity) < 0.2) &&
        gridMarchW(m_position, m_velocity * dT, terrain, &collisionDist, &blockPos) &&
        collisionDist <= pow(2, 1/2) / 2) {
        if (!inAir) {
            m_velocity.y += JUMP_CONST;
            inAir = true;
        }
    }

    float targetRot = std::atan2(direction.z, direction.x);
    whatIsMyRot = -1.f * targetRot + M_PI / 2;

    accelDir.y = 0;

    // renormalize acceleration direction
    if (glm::length(accelDir) > 1) {
        accelDir = glm::normalize(accelDir);
    }

    // add new acceleration
    m_acceleration += ACCELERATION_CONST * accelDir;
}

void Cow::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
}

void Cow::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
}
void Cow::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
}
void Cow::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
}
void Cow::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
}
void Cow::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
}
void Cow::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
}
void Cow::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
}
void Cow::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
}
void Cow::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
}
void Cow::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
}
void Cow::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
}
void Cow::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
}

void Cow::updatePlayerPos(glm::vec3 newPos) {
    this->playerPos = newPos;
}

void Cow::summon() {
    this->m_position = this->playerPos - glm::vec3(0, -2, 10);
}

glm::vec3 Cow::getMPos() const {
    return this->m_position;
}

float Cow::getMyRot() const {
    return this->whatIsMyRot;
}

bool Cow::isMoving() const {
    return glm::length(this->m_velocity) > 0.01;
}

void Cow::animateLegs(float currentTime) {
    currentTime /= 100;
    float cosDT = glm::mix(-40.f, 40.f, (glm::cos(currentTime) + 1.f) / 2.f),
        sinDT = glm::mix(-40.f, 40.f, (glm::sin(currentTime) + 1.f) / 2.f);
    Joint* thighR = this->joint->getChildren()[0].get();
    Joint* thighL = this->joint->getChildren()[1].get();
    Joint* shoulderR = this->joint->getChildren()[2]->getChildren()[0]->getChildren()[1].get();
    Joint* shoulderL = this->joint->getChildren()[2]->getChildren()[0]->getChildren()[2].get();
    thighR->setXRot(cosDT);
    shoulderL->setXRot(cosDT);
    thighL->setXRot(sinDT);
    shoulderR->setXRot(sinDT);
}
