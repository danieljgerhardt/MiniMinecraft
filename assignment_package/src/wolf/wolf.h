#ifndef WOLF_H
#define WOLF_H

#include "EntityDrawable.h"
#include "scene/entity.h"
#include "scene/terrain.h"
#include "smartpointerhelp.h"
#include "component.h"
#include <QFile>

class Wolf : public EntityDrawable, public Entity {
private:
    std::vector<uPtr<Vertex>> vertices;
    std::vector<uPtr<Face>> faces;
    std::vector<uPtr<HalfEdge>> hes;

    uPtr<Joint> joint;

    bool hasBeenSkinned;

    int calcTotalIndices();

    const Terrain &terrain;

    glm::vec3 playerPos;
    glm::vec3 m_velocity, m_acceleration;

    bool inAir;

    void collideAndMove(const Terrain& terrain, float dt);
    void computePhysics(float dT, const Terrain &terrain);
    void genInputs(float dT);

    float whatIsMyRot;
public:
    Wolf(OpenGLContext *context, std::vector<glm::vec3>& vertices,
        std::vector<glm::vec3>& normals, std::vector<std::vector<int>>& faces, glm::vec3 pos, const Terrain &t);
    ~Wolf();

    void create() override;
    GLenum drawMode() override;

    void addFace(uPtr<Face> fPtr);
    void addVertex(uPtr<Vertex> vPtr);
    void addHE(uPtr<HalfEdge> hePtr);

    std::vector<uPtr<Vertex>>& getVertices();
    std::vector<uPtr<Face>>& getFaces();
    std::vector<uPtr<HalfEdge>>& getHes();

    bool hasVertices() const;

    void setRootJoint(uPtr<Joint> newRoot);
    void findVertJointsRec(Vertex* v, Joint* curr, std::array<Joint*, 2>& jointArr, std::array<float, 2>& dists);
    void skinWolf();
    bool hasJoints() const;
    uPtr<Joint>& getRoot();
    bool skinned() const;

    void tick(float dT, qint64 currTime);
    void tick(float dT, InputBundle& input) override;

    void moveAlongVector(glm::vec3 dir) override;
    void moveForwardLocal(float amount) override;
    void moveRightLocal(float amount) override;
    void moveUpLocal(float amount) override;
    void moveForwardGlobal(float amount) override;
    void moveRightGlobal(float amount) override;
    void moveUpGlobal(float amount) override;
    void rotateOnForwardLocal(float degrees) override;
    void rotateOnRightLocal(float degrees) override;
    void rotateOnUpLocal(float degrees) override;
    void rotateOnForwardGlobal(float degrees) override;
    void rotateOnRightGlobal(float degrees) override;
    void rotateOnUpGlobal(float degrees) override;

    void updatePlayerPos(glm::vec3 newPos);

    void summon();
    glm::vec3 getMPos() const;

    float getMyRot() const;

    bool isMoving() const;
    void animateLegs(float currentTime);
};

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        return h1 ^ h2;
    }
};

#endif // WOLF_H
