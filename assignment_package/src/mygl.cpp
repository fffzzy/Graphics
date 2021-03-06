#include "mygl.h"
#include "scene/terrain.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_ppShader(),
      m_geomQuad(this),
      fb(this,0,0,0),
      m_worldAxes(this),
      m_progLambert(this), m_progFlat(this), m_diffuseTexture(this),
      m_terrain(this), m_player(glm::vec3(32.f, 200.f, 32.f), m_terrain),
      m_currFrameTime(QDateTime::currentMSecsSinceEpoch()),
      m_prevFrameTime(QDateTime::currentMSecsSinceEpoch()),
      accumulativeRotationOnRight(0.f), m_time(0.f)
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
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);
    m_diffuseTexture.create(":/textures/minecraft_textures_all.png");
    m_diffuseTexture.load(0);

    fb = FrameBuffer(this, this->width(), this->height(), this->devicePixelRatio());
    fb.create();

    //Create the instance of the world axes
    m_worldAxes.createVBOdata();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");

    // Set a color with which to draw geometry.
    // This will ultimately not be used when you change
    // your program to render Chunks with vertex colors
    // and UV coordinates
    m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));

    createShaders();


    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

//    m_terrain.CreateTestScene();
}


void MyGL::createShaders()

{
    std::shared_ptr<PPShader> grey = std::make_shared<PPShader>(this);
    grey->create(":/glsl/post/passthrough.vert.glsl", ":/glsl/post/greyscale.frag.glsl");
    m_ppShader.push_back(grey);
    //m_ppShader = std::make_shared<PPShader>(this);
    //std::shared_ptr<PPShader> grey = std::make_shared<PPShader>(this);

    mp_progPostprocessCurrent = m_ppShader[0].get();

}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.

    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);
    fb.resize(this->width(), this->height(), this->devicePixelRatio());

    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    m_player.mcr_posPrev = m_player.mcr_position;
    m_currFrameTime = QDateTime::currentMSecsSinceEpoch();
    float dT =(m_currFrameTime - m_prevFrameTime) * 0.1f;
    m_player.tick(dT, m_inputs);
//    cout << "tick()" << endl;
    m_terrain.multithreadedWork(m_player.mcr_position, m_player.mcr_posPrev, dT);
    m_progLambert.setTime(m_time); // Set time in shader
    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
    m_prevFrameTime = m_currFrameTime;
    m_time++; // Update time
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    //fb.bindFrameBuffer();

    glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());

    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setModelMatrix(glm::mat4());

    //this->m_terrain.expandTerrain(m_player.mcr_position.x, m_player.mcr_position.z);

    m_diffuseTexture.bind(0);

    renderTerrain();

    glDisable(GL_DEPTH_TEST);
    m_progFlat.setModelMatrix(glm::mat4());
    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progFlat.draw(m_worldAxes);
    glEnable(GL_DEPTH_TEST);

    //performPostprocessRenderPass();
}

void MyGL::performPostprocessRenderPass()
{
    // Render the frame buffer as a texture on a screen-size quad

    // Tell OpenGL to render to the viewport's frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    // Render on the whole framebuffer, complete from the lower left corner to the upper right
    glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    fb.bindToTextureSlot(0);
    mp_progPostprocessCurrent->draw(m_geomQuad, 0);
}

// TODO: Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info)
void MyGL::renderTerrain() {
    int xmin = 16 * (glm::floor(this->m_player.mcr_position.x / 16.f) - 1);
    int xmax = 16 * (glm::floor(this->m_player.mcr_position.x / 16.f) + 2);

    int zmin = 16 * (glm::floor(this->m_player.mcr_position.z / 16.f) - 1);
    int zmax = 16 * (glm::floor(this->m_player.mcr_position.z / 16.f) + 2);
    m_terrain.draw(xmin, xmax, zmin, zmax, &m_progLambert);
}


void MyGL::keyPressEvent(QKeyEvent *e) {
    float amount = 2.f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = true;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = true;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = true;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = true;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = true;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = true;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = true;
    } else if (e->key() == Qt::Key_F) {
        m_player.toggleFlight();
    }
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = false;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = false;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = false;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = false;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = false;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = false;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = false;
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    // TODO
    QPoint currentPosition = e->pos();
    QPoint center(width() / 2, height() / 2);
    float theta = static_cast<float>(currentPosition.x() - center.x());
    float phi = static_cast<float>(currentPosition.y() - center.y());
    theta *= sensitivity;
    phi *= sensitivity;
    accumulativeRotationOnRight += phi;
    if (accumulativeRotationOnRight > 90.f) {
        float diff = 90.f - accumulativeRotationOnRight;
        phi += diff;
        accumulativeRotationOnRight = 90.f;
    } else if (accumulativeRotationOnRight < -90.f) {
        float diff = -90.f - accumulativeRotationOnRight;
        phi += diff;
        accumulativeRotationOnRight = -90.f;
    }
    m_player.rotateOnRightLocal(m_inputs.mouseY);
    m_player.rotateOnUpGlobal(m_inputs.mouseX);
    m_inputs.mouseY = -phi;
    m_inputs.mouseX = -theta;
    moveMouseToCenter();


}

void MyGL::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_player.removeBlock();
    } else if (e->button() == Qt::RightButton) {
        m_player.addBlock();
    }
}


