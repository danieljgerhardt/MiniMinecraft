#include "joint.h"

#define PI 3.1415926535f

Joint::Joint() :
    name(""), parent(nullptr), pos_relative(glm::vec3()), rot(glm::quat()), bindMat(glm::mat4()), id(topId + 1)
{
    topId++;
}


Joint::Joint(glm::vec3 pos) :
    name(""), parent(nullptr), pos_relative(pos), rot(glm::quat()), bindMat(glm::mat4()), id(topId + 1)
{
    topId++;
}

Joint::Joint(QString name, glm::vec3 pos, glm::quat rot) :
    name(name), parent(nullptr), pos_relative(pos), rot(rot), id(topId + 1)
{
    topId++;
}

Joint::~Joint() {}

void Joint::addChild(uPtr<Joint> newChild) {
    this->children.push_back(std::move(newChild));
}

void Joint::setParent(Joint* p) {
    this->parent = p;
}

void Joint::updateBindMat() {
    this->bindMat = glm::inverse(this->getOverallTransformation());
    for (auto& child : children) {
        child->updateBindMat();
    }
}

glm::mat4 Joint::getBind() {
    return this->bindMat;
}

glm::mat4 Joint::getLocalTransformation() const {
    return glm::translate(glm::mat4(), pos_relative) * glm::toMat4(rot);
}

glm::mat4 Joint::getOverallTransformation() const {
    if (!parent) {
        return this->getLocalTransformation();
    }
    return parent->getOverallTransformation() * this->getLocalTransformation();
}

QString Joint::getName() const {
    return this->name;
}

int Joint::getId() const {
    return this->id;
}

std::vector<uPtr<Joint>>& Joint::getChildren() {
    return children;
}

void Joint::posXRot() {
    this->rot = glm::angleAxis(glm::radians(5.f), glm::vec3(1, 0, 0)) * this->rot;
}

void Joint::posYRot() {
    this->rot = glm::angleAxis(glm::radians(5.f), glm::vec3(0, 1, 0)) * this->rot;
}

void Joint::posZRot() {
    this->rot = glm::angleAxis(glm::radians(5.f), glm::vec3(0, 0, 1)) * this->rot;
}

void Joint::negXRot() {
    this->rot = glm::angleAxis(glm::radians(-5.f), glm::vec3(1, 0, 0)) * this->rot;
}

void Joint::negYRot() {
    this->rot = glm::angleAxis(glm::radians(-5.f), glm::vec3(0, 1, 0)) * this->rot;
}

void Joint::negZRot() {
    this->rot = glm::angleAxis(glm::radians(-5.f), glm::vec3(0, 0, 1)) * this->rot;
}

void Joint::setX(float x) {
    this->pos_relative.x = x;
}

void Joint::setY(float y) {
    this->pos_relative.y = y;
}

void Joint::setZ(float z) {
    this->pos_relative.z = z;
}

void Joint::modYRot(float y) {
    this->rot = glm::angleAxis(glm::radians(y), glm::vec3(0, 1, 0)) * this->rot;
}

void Joint::modXRot(float x) {
    this->rot = glm::angleAxis(glm::radians(x), glm::vec3(1, 0, 0)) * this->rot;
}

void Joint::setXRot(float x) {
    this->rot = glm::angleAxis(glm::radians(x), glm::vec3(1, 0, 0));
}

void Joint::modPos(glm::vec3 pos) {
    this->pos_relative += pos;
}

int Joint::topId = 0;
