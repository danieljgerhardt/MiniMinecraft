#include "component.h"

HalfEdge::HalfEdge()
    : next(nullptr), sym(nullptr), face(nullptr), v(nullptr), id(topId + 1)
{
    topId++;
}

HalfEdge::~HalfEdge() {}

void HalfEdge::setSym(HalfEdge* he) {
    he->sym = this;
    this->sym = he;
}

void HalfEdge::setFace(Face* f) {
    f->he = this;
    this->face = f;
}

void HalfEdge::setVertex(Vertex* v) {
    this->v = v;
}

Vertex* HalfEdge::getVertex() const {
    return this->v;
}

HalfEdge* HalfEdge::getNext() const {
    return this->next;
}

HalfEdge* HalfEdge::getSym() const {
    return this->sym;
}

void HalfEdge::setNext(HalfEdge* he) {
    this->next = he;
}

Face* HalfEdge::getFace() const {
    return this->face;
}

int HalfEdge::getId() const {
    return this->id;
}


Vertex::Vertex()
    : pos(glm::vec3()), he(nullptr), id(topId + 1)
{
    topId++;
}

Vertex::Vertex(Vertex& v)
    : pos(v.pos), he(v.he), id(v.id)
{}

Vertex::~Vertex() {}

glm::vec3 Vertex::getPos() const {
    return this->pos;
}

int Vertex::getId() const {
    return this->id;
}

HalfEdge* Vertex::getHE() const {
    return this->he;
}

void Vertex::setX(float x) {
    this->pos.x = x;
}

void Vertex::setY(float y) {
    this->pos.y = y;
}

void Vertex::setZ(float z) {
    this->pos.z = z;
}

void Vertex::addInfluence(Joint* j, float weight) {
    this->influences.push_back(std::pair<Joint*, float>(j, weight));
}

std::vector<std::pair<Joint*, float>>& Vertex::getInfluences() {
    return this->influences;
}

Face::Face()
    : he(nullptr), color(glm::vec3()), id(topId + 1)
{
    topId++;
}

Face::~Face() {}

void Face::setEdge(HalfEdge* he) {
    this->he = he;
    he->face = this;
}

void Face::setR(float r) {
    this->color.r = r;
}

void Face::setG(float g) {
    this->color.g = g;
}

void Face::setB(float b) {
    this->color.b = b;
}

HalfEdge* Face::getHE() const {
    return this->he;
}

glm::vec3 Face::getColor() const {
    return this->color;
}

int HalfEdge::topId = 0;
int Vertex::topId = 0;
int Face::topId = 0;
