#ifndef MYGL_H
#define MYGL_H

#include "openglcontext.h"
#include "scene/postprocessshader.h"
#include "shaderprogram.h"
#include "scene/worldaxes.h"
#include "scene/camera.h"
#include "scene/terrain.h"
#include "scene/player.h"
#include "framebuffer.h"
#include "quad.h"
#include "newtexture.h"
#include "wolf/wolf.h"
#include "wolf/cow.h"
#include "inventory.h"

#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <smartpointerhelp.h>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

enum State : unsigned char {
    UNINITALIZED, LOADING, PLAYING, SAVING, CLOSING
};

class MyGL : public OpenGLContext
{
    Q_OBJECT
private:
    WorldAxes m_worldAxes; // A wireframe representation of the world axes. It is hard-coded to sit centered at (32, 128, 32).
    ShaderProgram m_progLambert;// A shader program that uses lambertian reflection
    ShaderProgram m_progFlat;// A shader program that uses "flat" reflection (no shadowing at all)
    ShaderProgram m_progInstanced;// A shader program that is designed to be compatible with instanced rendering
    ShaderProgram m_progSkeleton;

    PostProcessShader* mp_progPostCurr;
    PostProcessShader m_progPostNoop;
    PostProcessShader m_progPostLava;
    PostProcessShader m_progPostWater;
    Quad m_geomQuad;

    GLuint vao; // A handle for our vertex array object. This will store the VBOs created in our geometry classes.
                // Don't worry too much about this. Just know it is necessary in order to render geometry.

    uPtr<Terrain> mp_terrain; // All of the Chunks that currently comprise the world.
    uPtr<Player> mp_player; // The entity controlled by the user. Contains a camera to display what it sees as well.
    InputBundle m_inputs; // A collection of variables to be updated in keyPressEvent, mouseMoveEvent, mousePressEvent, etc.

    NewTexture m_blockTextureAtlas;

    QTimer m_timer; // Timer linked to tick(). Fires approximately 60 times per second.

    void moveMouseToCenter(); // Forces the mouse position to the screen's center. You should call this
                              // from within a mouse move event after reading the mouse movement so that
                              // your mouse stays within the screen bounds and is always read.

    void sendPlayerDataToGUI() const;

    qint64 m_lastFrameTime;
    qint64 m_initalTime;

    void processInputs();

    FrameBuffer buffer;
    GLuint m_renderedTexture;

    void readWolf(QString directory);
    uPtr<Wolf> wolf;

    void readCow(QString directory);
    uPtr<Cow> cow;

    uPtr<Joint> readJoints(QFile file, bool animalType);
    uPtr<Joint> readJointsRec(QJsonObject joint, Joint* parent, bool animalType);
    
    State state;
    int tickTimer;
    uPtr<Camera> skyCamera;

    Inventory inventory;
    NewTexture inventorySlotTexture;
    ShaderProgram guiShader;

public:
    explicit MyGL(QWidget *parent = nullptr);
    ~MyGL();

    // Called once when MyGL is initialized.
    // Once this is called, all OpenGL function
    // invocations are valid (before this, they
    // will cause segfaults)
    void initializeGL() override;
    // Called whenever MyGL is resized.
    void resizeGL(int w, int h) override;
    // Called whenever MyGL::update() is called.
    // In the base code, update() is called from tick().
    void paintGL() override;

    // Called from paintGL().
    // Calls Terrain::draw().
    void renderTerrain();

    void load(QString savename);
    void save();

protected:
    // Automatically invoked when the user
    // presses a key on the keyboard
    void keyPressEvent(QKeyEvent *e) override;
    // for when key is released
    void keyReleaseEvent(QKeyEvent *e) override;

    // Automatically invoked when the user
    // moves the mouse
    void mouseMoveEvent(QMouseEvent *e) override;
    // Automatically invoked when the user
    // presses a mouse button
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private slots:
    void tick(); // Slot that gets called ~60 times per second by m_timer firing.]

signals:
    void sig_sendPlayerPos(QString) const;
    void sig_sendPlayerVel(QString) const;
    void sig_sendPlayerAcc(QString) const;
    void sig_sendPlayerLook(QString) const;
    void sig_sendPlayerChunk(QString) const;
    void sig_sendPlayerTerrainZone(QString) const;

    void sig_toggleProgress();
    void sig_quit();
    void sig_sendFPS(float);
};


#endif // MYGL_H
