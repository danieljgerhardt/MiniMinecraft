#ifndef JOINT_H
#define JOINT_H

#include <QString>
#include "drawable.h"
#include "smartpointerhelp.h"
#include "la.h"
#include <QTreeWidgetItem>
#include <cmath>
#include <iostream>

class Joint {
private:
    QString name;
    Joint* parent;
    std::vector<uPtr<Joint>> children;
    glm::vec3 pos_relative;
    glm::quat rot;
    glm::mat4 bindMat;
    int id;
public:
    Joint();
    Joint(glm::vec3 pos);
    Joint(QString name, glm::vec3 pos, glm::quat rot);
    ~Joint();

    void addChild(uPtr<Joint> newChild);
    void setParent(Joint* p);

    void updateBindMat();
    glm::mat4 getBind();

    glm::mat4 getLocalTransformation() const;
    glm::mat4 getOverallTransformation() const;

    QString getName() const;
    int getId() const;

    std::vector<std::unique_ptr<Joint>>& getChildren();

    void posXRot();
    void posYRot();
    void posZRot();
    void negXRot();
    void negYRot();
    void negZRot();

    void setX(float x);
    void setY(float y);
    void setZ(float z);

    void modYRot(float y);
    void modXRot(float x);

    void setXRot(float x);

    float getYRot();

    void modPos(glm::vec3 mod);

    static int topId;
};

#endif // JOINT_H
