#ifndef COMPONENT_H
#define COMPONENT_H

#include "joint.h"
#include "qlistwidget.h"
#include <la.h>

class Vertex;
class HalfEdge;
class Face;

class HalfEdge : public QListWidgetItem {
private:
    HalfEdge* next;
    HalfEdge* sym;
    Face* face;
    Vertex* v;
    int id;
public:
    HalfEdge();
    ~HalfEdge();
    static int topId;
    void setSym(HalfEdge* he);
    void setFace(Face* f);
    void setVertex(Vertex* v);
    void setNext(HalfEdge* he);
    Vertex* getVertex() const;
    HalfEdge* getNext() const;
    HalfEdge* getSym() const;
    Face* getFace() const;
    int getId() const;

    friend class Vertex;
    friend class Face;
    friend class Wolf;
    friend class Cow;
};

class Vertex : public QListWidgetItem {
private:
    glm::vec3 pos;
    glm::vec3 nor;
    HalfEdge* he;
    int id;

    //added for hw7
    std::vector<std::pair<Joint*, float>> influences;
public:
    Vertex();
    Vertex(Vertex&);
    ~Vertex();
    static int topId;
    glm::vec3 getPos() const;
    int getId() const;
    HalfEdge* getHE() const;

    void setX(float x);
    void setY(float y);
    void setZ(float z);

    void addInfluence(Joint* j, float weight);
    std::vector<std::pair<Joint*, float>>& getInfluences();

    friend class HalfEdge;
    friend class Face;
    friend class Wolf;
    friend class Cow;
};

class Face : public QListWidgetItem {
private:
    HalfEdge* he;
    glm::vec3 color;
    int id;
public:
    Face();
    ~Face();
    static int topId;
    void setEdge(HalfEdge* he);
    glm::vec3 getColor() const;
    HalfEdge* getHE() const;

    void setR(float r);
    void setG(float g);
    void setB(float b);

    friend class HalfEdge;
    friend class Vertex;
    friend class Wolf;
    friend class Cow;
};

#endif // COMPONENT_H
