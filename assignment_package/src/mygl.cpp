#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QFile>

#define WOLF_MINIMIZER 0.14f
#define COW_MINIMIZER 1.f
#define BLOCK_TEXTURE_SLOT 1
#define INVENTORY_TEXTURE_SLOT 2

MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_worldAxes(this),
      m_progLambert(this), m_progFlat(this), m_progInstanced(this), m_progSkeleton(this),
      mp_progPostCurr(nullptr), m_progPostNoop(this), m_progPostLava(this), m_progPostWater(this), m_geomQuad(this),
      mp_terrain(nullptr), mp_player(nullptr), m_inputs(),
      m_lastFrameTime(QDateTime::currentMSecsSinceEpoch()),
      buffer(this, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio(), this->devicePixelRatio()),
    m_blockTextureAtlas(this), m_initalTime(m_lastFrameTime), state(UNINITALIZED), tickTimer(0), skyCamera(nullptr),
    inventory(this), inventorySlotTexture(this), guiShader(this)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    // set alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instance of the world axes
    m_worldAxes.createVBOdata();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");

    // create and set up skeleton shader
    m_progSkeleton.create(":glsl/skeleton.vert.glsl", ":/glsl/skeleton.frag.glsl");

    // create and set up post process shaders
    m_progPostWater.create(":/glsl/passthrough.vert.glsl", ":/glsl/water.frag.glsl");
    m_progPostLava.create(":/glsl/passthrough.vert.glsl", ":/glsl/lava.frag.glsl");
    m_progPostNoop.create(":/glsl/passthrough.vert.glsl", ":/glsl/noop.frag.glsl");

    guiShader.create(":/glsl/gui.vert.glsl", ":/glsl/gui.frag.glsl");

    mp_progPostCurr = &m_progPostNoop;

    //wolf time
    QString path = "../assignment_package/objs/wolf.obj";
    this->readWolf(path);
    this->wolf->create();

    //set up wolf joints
    path = "../assignment_package/jsons/wolf_skeleton.json";
    uPtr<Joint> wolfRootJoint = readJoints(QFile(path), false);
    wolf->setRootJoint(std::move(wolfRootJoint));
    wolf->skinWolf();
    this->wolf->create();

    path = "../assignment_package/objs/cow_mc.obj";
    this->readCow(path);
    this->cow->create();

    path = "../assignment_package/jsons/wolf_skeleton.json";
    uPtr<Joint> cowRootJoint = readJoints(QFile(path), true);
    cow->setRootJoint(std::move(cowRootJoint));
    cow->skinCow();
    this->cow->create();

    // Set a color with which to draw geometry.
    // This will ultimately not be used when you change
    // your program to render Chunks with vertex colors
    // and UV coordinates
    m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));

    // Set model matrix for m_progLambert so we do not need to compute a model matrix
    // every time we draw a chunk
    m_progLambert.setModelMatrix(glm::mat4(1.f));

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);


    m_progSkeleton.setModelMatrix(glm::mat4(1.f));

    //create m_geomQuad and FrameBuffer obj
    m_geomQuad.create();
    buffer.create();
    m_progPostWater.setupMemberVars();
    m_progPostLava.setupMemberVars();
    m_progPostNoop.setupMemberVars();

    m_blockTextureAtlas.create("../textures/minecraft_textures_all.png");
    m_blockTextureAtlas.load(BLOCK_TEXTURE_SLOT);
    m_progLambert.setTexture(BLOCK_TEXTURE_SLOT);
    //m_progInstanced.setTexture(BLOCK_TEXTURE_SLOT);
    inventorySlotTexture.create("../textures/item_frame.png");
    inventorySlotTexture.load(INVENTORY_TEXTURE_SLOT);

    inventory.initalize();

    //m_terrain.CreateTestScene();
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    mp_player->setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = mp_player->mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);
    m_progSkeleton.setViewProjMatrix(viewproj);

    buffer.resize(this->width(), this->height(), this->devicePixelRatio());
    buffer.destroy();
    buffer.create();
    m_progPostWater.setDimensions(glm::ivec2(w * this->devicePixelRatio(), h * this->devicePixelRatio()));
    m_progPostLava.setDimensions(glm::ivec2(w * this->devicePixelRatio(), h * this->devicePixelRatio()));
    m_progPostNoop.setDimensions(glm::ivec2(w * this->devicePixelRatio(), h * this->devicePixelRatio()));

    printGLErrorLog();
}

// process inputs that effect different classes (entity to terrain, entity to entity)
void MyGL::processInputs() {
    if (m_inputs.numPressed) {
        inventory.selectSlot(m_inputs.num);
        m_inputs.numPressed = false;
    }

    if (m_inputs.leftClick) {
        mp_player->breakBlock(*mp_terrain);
        m_inputs.leftClick = false;
    }
    if (m_inputs.rightClick) {
        BlockType block = inventory.getSelected();
        if (block == BONE) {
            this->wolf->summon();
        } else {
            mp_player->placeBlock(*mp_terrain, block);
        }
        m_inputs.rightClick = false;
    }
}

// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    // calculate change in time
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    float dt = (currentTime - m_lastFrameTime) * 0.001f;
    m_lastFrameTime = currentTime;
    tickTimer--;
    float entityDt = glm::min(0.0166666f, dt);
    emit sig_sendFPS(dt);

    switch (state) {
    case PLAYING:
        // process top level inputs
        processInputs();

        // update entities
        mp_player->tick(entityDt, m_inputs);

        // Expands terrain if player is near unloaded chunks
        mp_terrain->tick(mp_player->mcr_position, dt);
        
        mp_terrain->growTrees(QDateTime::currentMSecsSinceEpoch());

        wolf->updatePlayerPos(mp_player->mcr_position);
        wolf->tick(entityDt, currentTime);

        cow->updatePlayerPos(mp_player->mcr_position);
        cow->tick(entityDt, currentTime);

        update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
        sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data

        if (mp_player->getCamInWater()) {
            this->mp_progPostCurr = &this->m_progPostWater;
        } else if (mp_player->getCamInLava()) {
            this->mp_progPostCurr = &this->m_progPostLava;
        } else {
            this->mp_progPostCurr = &this->m_progPostNoop;
        }
        break;
        
    case LOADING:
        mp_terrain->tick(mp_player->mcr_position, dt);
        update();
    case CLOSING:
    case SAVING:
        if (tickTimer <= 0 && mp_terrain->threadsIdle()){
            tickTimer = 60 * 3;
            switch (state) {
            case CLOSING:
                QApplication::exit(0);
                break;
            case SAVING:
                emit sig_toggleProgress();
                state = CLOSING;
                break;
            case LOADING:
                emit sig_toggleProgress();
                state = PLAYING;
                update();
                break;
            }
        }

        break;
    case UNINITALIZED:
        break;
    }

}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(mp_player->posAsQString());
    emit sig_sendPlayerVel(mp_player->velAsQString());
    emit sig_sendPlayerAcc(mp_player->accAsQString());
    emit sig_sendPlayerLook(mp_player->lookAsQString());
    glm::vec2 pPos(mp_player->mcr_position.x, mp_player->mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {

    //set some buffer stuff
    buffer.bindFrameBuffer();
    glViewport(0, 0, this->width(), this->height());

    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //draw the geom
    if (state == LOADING) {
        m_progFlat.setViewProjMatrix(skyCamera->getViewProj());
        m_progLambert.setViewProjMatrix(skyCamera->getViewProj());
    } else {
        m_progFlat.setViewProjMatrix(mp_player->mcr_camera.getViewProj());
        m_progLambert.setViewProjMatrix(mp_player->mcr_camera.getViewProj());
        m_progSkeleton.setViewProjMatrix(mp_player->mcr_camera.getViewProj());
        //m_progInstanced.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    }

    m_progLambert.setTime(m_initalTime - m_lastFrameTime);

    m_blockTextureAtlas.bind(BLOCK_TEXTURE_SLOT);

    renderTerrain();

    //wolf draw
    if (state == PLAYING) {
        int mCount = 0;
        std::array<glm::mat4, 100> binds, transforms;
        std::vector<Joint*> queue;
        queue.push_back(wolf->getRoot().get());
        //data structures moment -- do while for level order traversal
        do {
            Joint* curr = queue[0];

            //update binds and transforms mat for prog skelly
            binds[curr->getId() - 1] = curr->getBind();
            transforms[curr->getId() - 1] = curr->getOverallTransformation();
            mCount++;

            //add children to queue
            for (uPtr<Joint>& child : curr->getChildren()) {
                queue.push_back(child.get());
            }
            queue.erase(queue.begin());
        } while (!queue.empty());

        glm::mat4 model = glm::translate(glm::mat4(1.0f), this->wolf->getMPos()) *
                          glm::rotate(glm::mat4(1.0f), this->wolf->getMyRot(), glm::vec3(0, 1, 0));
        m_progSkeleton.setModelMatrix(model);
        m_progSkeleton.setBinds(binds, mCount);
        m_progSkeleton.setTransforms(transforms, mCount);
        m_progSkeleton.draw(*wolf);
    }

    //cow draw
    if (state == PLAYING) {
        int mCount = 0;
        std::array<glm::mat4, 100> binds, transforms;
        std::vector<Joint*> queue;
        queue.push_back(cow->getRoot().get());
        //data structures moment -- do while for level order traversal
        do {
            Joint* curr = queue[0];

            //update binds and transforms mat for prog skelly
            binds[curr->getId() - 1] = curr->getBind();
            transforms[curr->getId() - 1] = curr->getOverallTransformation();
            mCount++;

            //add children to queue
            for (uPtr<Joint>& child : curr->getChildren()) {
                queue.push_back(child.get());
            }
            queue.erase(queue.begin());
        } while (!queue.empty());

        glm::mat4 model = glm::translate(glm::mat4(1.0f), this->cow->getMPos()) *
                          glm::rotate(glm::mat4(1.0f), this->cow->getMyRot(), glm::vec3(0, 1, 0));
        m_progSkeleton.setModelMatrix(model);
        m_progSkeleton.setBinds(binds, mCount);
        m_progSkeleton.setTransforms(transforms, mCount);
        m_progSkeleton.draw(*cow);
    }

    //do more buffer stuff
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glViewport(0, 0, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    buffer.bindToTextureSlot(0);
    mp_progPostCurr->draw(m_geomQuad, 0);

    if (state == PLAYING) {
      inventorySlotTexture.bind(INVENTORY_TEXTURE_SLOT);
      m_blockTextureAtlas.bind(BLOCK_TEXTURE_SLOT);
      inventory.draw(&guiShader, BLOCK_TEXTURE_SLOT,INVENTORY_TEXTURE_SLOT);
    }
}

// TODO: Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info)
void MyGL::renderTerrain() {
    float x = mp_player->mcr_position.x, z = mp_player->mcr_position.z;
    int minX = static_cast<int>(glm::floor((x - (64 * DRAW_RADIUS)) / 64.f)), minZ = static_cast<int>(glm::floor((z - (64 * DRAW_RADIUS)) / 64.f));
    int maxX = static_cast<int>(glm::ceil((x + (64 * DRAW_RADIUS)) / 64.f)), maxZ = static_cast<int>(glm::ceil((z + (64 * DRAW_RADIUS)) / 64.f));
    mp_terrain->draw(minX * 64, maxX * 64, minZ * 64, maxZ * 64, &m_progLambert);
}

void MyGL::keyPressEvent(QKeyEvent *e) {
    switch (e->key()) {
    case (Qt::Key_Escape):
        emit sig_quit();
        break;
    case (Qt::Key_W):
        m_inputs.wPressed = true;
        break;
    case (Qt::Key_A):
        m_inputs.aPressed = true;
        break;
    case (Qt::Key_S):
        m_inputs.sPressed = true;
        break;
    case (Qt::Key_D):
        m_inputs.dPressed = true;
        break;
    case (Qt::Key_Q):
        m_inputs.qPressed = true;
        break;
    case (Qt::Key_E):
        m_inputs.ePressed = true;
        break;
    case (Qt::Key_F):
        m_inputs.fPressed = true;
        break;
    case (Qt::Key_H):
        this->wolf->summon();
        this->cow->summon();
        break;
    case (Qt::Key_Space):
        m_inputs.spacePressed = true;
        break;
    case (Qt::Key_1):
        m_inputs.numPressed = true;
        m_inputs.num = 1;
        break;
    case (Qt::Key_2):
        m_inputs.numPressed = true;
        m_inputs.num = 2;
        break;
    case (Qt::Key_3):
        m_inputs.numPressed = true;
        m_inputs.num = 3;
        break;
    case (Qt::Key_4):
        m_inputs.numPressed = true;
        m_inputs.num = 4;
        break;
    case (Qt::Key_5):
        m_inputs.numPressed = true;
        m_inputs.num = 5;
        break;
    case (Qt::Key_6):
        m_inputs.numPressed = true;
        m_inputs.num = 6;
        break;
    case (Qt::Key_7):
        m_inputs.numPressed = true;
        m_inputs.num = 7;
        break;
    case (Qt::Key_8):
        m_inputs.numPressed = true;
        m_inputs.num = 8;
        break;
    case (Qt::Key_9):
        m_inputs.numPressed = true;
        m_inputs.num = 9;
        break;
    case (Qt::Key_0):
        m_inputs.numPressed = true;
        m_inputs.num = 10;
        break;
    }



}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    switch (e->key()) {
    case (Qt::Key_W):
        m_inputs.wPressed = false;
        break;
    case (Qt::Key_A):
        m_inputs.aPressed = false;
        break;
    case (Qt::Key_S):
        m_inputs.sPressed = false;
        break;
    case (Qt::Key_D):
        m_inputs.dPressed = false;
        break;
    case (Qt::Key_Q):
        m_inputs.qPressed = false;
        break;
    case (Qt::Key_E):
        m_inputs.ePressed = false;
        break;
    case (Qt::Key_Space):
        m_inputs.spacePressed = false;
        break;
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    m_inputs.mouseX = e->pos().x() - (width() / 2);
    m_inputs.mouseY = e->pos().y() - (height() / 2);
    moveMouseToCenter();
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    switch (e->button()) {
    case (Qt::LeftButton):
        m_inputs.leftClick = true;
        break;
    case (Qt::RightButton):
        m_inputs.rightClick = true;
        break;
    }
}

void MyGL::mouseReleaseEvent(QMouseEvent *e) {
    switch (e->button()) {
    case (Qt::LeftButton):
        m_inputs.leftClick = false;
        break;
    case (Qt::RightButton):
        m_inputs.rightClick = false;
    }
}

void MyGL::readWolf(QString directory) {
    //https://forum.qt.io/topic/59249/read-textfile-and-display-on-qlabel-solved
    //help log 6:30 10/11
    QFile file = QFile(directory);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cout << "could not read" << std::endl;
        return;
    }
    QString string;
    QTextStream in(&file);
    QTextStream stream(&file);

    //get rid of header data
    for (int i = 0; i < 4; i++) {
        stream.readLine();
    }

    //vcetors to contain data that is read in
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<std::vector<int>> faces;

    while (!stream.atEnd()){
        string = stream.readLine();

        if (string.length() < 2) continue;

        if (string[0] == 'v' && string[1] == ' ') {
            //line is a vertex
            std::string temp = string.toStdString().substr(2); //skip the "v " prefix
            std::stringstream ss(temp);
            std::vector<float> v;
            float value;

            while (ss >> value) {
                v.push_back(value * WOLF_MINIMIZER);
            }
            vertices.push_back(glm::vec3(v[0], v[1], v[2]));
        } else if (string[0] == 'v' && string[1] == 'n') {
            //line is a normal
            std::string temp = string.toStdString().substr(3); //skip the "vn " prefix
            std::stringstream ss(temp);
            std::vector<float> v;
            float value;

            while (ss >> value) {
                v.push_back(value);
            }
            normals.push_back(glm::vec3(v[0], v[1], v[2]));
        } else if (string[0] == 'f') {
            //line is a face
            std::string temp = string.toStdString().substr(2); //skip the "f " prefix
            std::stringstream ss(temp);
            std::vector<int> vertexIndices;
            std::string s;
            int num1, num2, num3;
            char slash1, slash2;
            //man i love string steams :)
            while (ss >> num1 >> slash1 >> num2 >> slash2 >> num3) {
                //subtract 1 because obj isn't 0 indexed
                vertexIndices.push_back(num1 - 1);
            }

            faces.push_back(vertexIndices);
        }
    }

    //use mesh constructor to turn this data into a mesh
    this->wolf = mkU<Wolf>(this, vertices, normals, faces, glm::vec3(48.f, 180.f, 38.f), *mp_terrain);

    file.close();
}

void MyGL::readCow(QString directory) {
    //https://forum.qt.io/topic/59249/read-textfile-and-display-on-qlabel-solved
    //help log 6:30 10/11
    QFile file = QFile(directory);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cout << "could not read" << std::endl;
        return;
    }
    QString string;
    QTextStream in(&file);
    QTextStream stream(&file);

    //get rid of header data
    for (int i = 0; i < 4; i++) {
        stream.readLine();
    }

    //vcetors to contain data that is read in
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<std::vector<int>> faces;

    while (!stream.atEnd()){
        string = stream.readLine();

        if (string.length() < 2) continue;

        if (string[0] == 'v' && string[1] == ' ') {
            //line is a vertex
            std::string temp = string.toStdString().substr(2); //skip the "v " prefix
            std::stringstream ss(temp);
            std::vector<float> v;
            float value;

            while (ss >> value) {
                v.push_back(value * COW_MINIMIZER);
            }
            vertices.push_back(glm::vec3(v[0], v[1], v[2]));
        } else if (string[0] == 'v' && string[1] == 'n') {
            //line is a normal
            std::string temp = string.toStdString().substr(3); //skip the "vn " prefix
            std::stringstream ss(temp);
            std::vector<float> v;
            float value;

            while (ss >> value) {
                v.push_back(value);
            }
            normals.push_back(glm::vec3(v[0], v[1], v[2]));
        } else if (string[0] == 'f') {
            //line is a face
            std::string temp = string.toStdString().substr(2); //skip the "f " prefix
            std::stringstream ss(temp);
            std::vector<int> vertexIndices;
            std::string s;
            int num1, num2, num3;
            char slash1, slash2;
            //man i love string steams :)
            while (ss >> num1 >> slash1 >> num2 >> slash2 >> num3) {
                //subtract 1 because obj isn't 0 indexed
                vertexIndices.push_back(num1 - 1);
            }

            faces.push_back(vertexIndices);
        }
    }

    //use mesh constructor to turn this data into a mesh
    this->cow = mkU<Cow>(this, vertices, normals, faces, glm::vec3(48.f, 180.f, 38.f), *mp_terrain);

    file.close();
}

//false = wolf, true = cow
uPtr<Joint> MyGL::readJoints(QFile file, bool animalType) {
    //https://stackoverflow.com/questions/15893040/how-to-create-read-write-json-files-in-qt5
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cout << "could not read" << std::endl;
        return nullptr;
    }
    QString string = file.readAll();
    file.close();
    QJsonDocument json = QJsonDocument::fromJson(string.toUtf8());
    QJsonObject object = json.object();
    QJsonObject rootJoint = object["root"].toObject();
    return readJointsRec(rootJoint, nullptr, animalType);
}

//false = wolf, true = cow
uPtr<Joint> MyGL::readJointsRec(QJsonObject joint, Joint* parent, bool animalType) {
    QJsonArray currPos = joint["pos"].toArray(), currRot = joint["rot"].toArray(), currChildren = joint["children"].toArray();

    QString name = joint["name"].toString();

    float mult = animalType ? COW_MINIMIZER : WOLF_MINIMIZER;

    glm::vec3 position = glm::vec3(currPos[0].toDouble() * mult, currPos[1].toDouble() * mult, currPos[2].toDouble() * mult);
    glm::vec3 axis = glm::vec3(currRot[1].toDouble(), currRot[2].toDouble(), currRot[3].toDouble());
    glm::quat rotation = glm::quat(glm::angleAxis(float(currRot[0].toDouble()), axis));

    uPtr<Joint> curr = mkU<Joint>(name, position, rotation);

    for (int i = 0; i < currChildren.size(); i++) {
        readJointsRec(currChildren[i].toObject(), curr.get(), animalType);
    }

    if (parent != nullptr) {
        curr->setParent(parent);
        parent->addChild(std::move(curr));
        return NULL;
    } else {
        return curr;
    }
}
    
void MyGL::load(QString savename) {
    QFile file("../saves/"+savename+"/"+savename+".save");
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    glm::vec3 pos;
    in >> pos.x >> pos.y >> pos.z;

    file.close();

    mp_terrain = mkU<Terrain>(this, savename);
    mp_player = mkU<Player>(pos,*mp_terrain);

    state = LOADING;
    tickTimer = 60 * 5;

    skyCamera = mkU<Camera>(glm::vec3(pos.x,425,pos.z));
    skyCamera->rotateOnRightGlobal(-90.0f);
}

void MyGL::save() {
    mp_terrain->save();
    state = SAVING;
    tickTimer = 60 * 5;
}

