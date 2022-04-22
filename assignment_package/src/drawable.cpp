#include "drawable.h"
#include <glm_includes.h>

Drawable::Drawable(OpenGLContext* context)
    : m_count(-1), m_bufIdx(), m_bufPos(), m_bufNor(), m_bufCol(),m_bufUV(),
      m_idxGenerated(false), m_posGenerated(false), m_norGenerated(false), m_colGenerated(false), m_uvGenerated(false),
      m_count_sec(-1), m_bufIdx_sec(), m_bufPos_sec(), m_bufNor_sec(),
      m_idxGenerated_sec(false), m_posGenerated_sec(false), m_norGenerated_sec(false), m_uvGenerated_sec(false),
      mp_context(context)
{}

Drawable::~Drawable()
{}


void Drawable::destroyVBOdata()
{
    mp_context->glDeleteBuffers(1, &m_bufIdx);
    mp_context->glDeleteBuffers(1, &m_bufPos);
    mp_context->glDeleteBuffers(1, &m_bufNor);
    mp_context->glDeleteBuffers(1, &m_bufUV);
    mp_context->glDeleteBuffers(1, &m_bufIdx_sec);
    mp_context->glDeleteBuffers(1, &m_bufPos_sec);
    mp_context->glDeleteBuffers(1, &m_bufNor_sec);
    mp_context->glDeleteBuffers(1, &m_bufUV_sec);
    mp_context->glDeleteBuffers(1, &m_bufCol);
    m_idxGenerated = m_posGenerated = m_norGenerated = m_colGenerated = m_idxGenerated_sec = m_posGenerated_sec = m_norGenerated_sec = false;
    m_count = -1;
    m_count_sec = -1;
}

GLenum Drawable::drawMode()
{
    // Since we want every three indices in bufIdx to be
    // read to draw our Drawable, we tell that the draw mode
    // of this Drawable is GL_TRIANGLES

    // If we wanted to draw a wireframe, we would return GL_LINES

    return GL_TRIANGLES;
}

int Drawable::elemCount()
{
    return m_count;
}


int Drawable::elemCount_sec()
{
    return m_count_sec;
}

void Drawable::generateIdx()
{
    m_idxGenerated = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdx);
}

void Drawable::generatePos()
{
    m_posGenerated = true;
    // Create a VBO on our GPU and store its handle in bufPos
    mp_context->glGenBuffers(1, &m_bufPos);
}

void Drawable::generateNor()
{
    m_norGenerated = true;
    // Create a VBO on our GPU and store its handle in bufNor
    mp_context->glGenBuffers(1, &m_bufNor);
}

void Drawable::generateCol()
{
    m_colGenerated = true;
    // Create a VBO on our GPU and store its handle in bufCol
    mp_context->glGenBuffers(1, &m_bufCol);
}

void Drawable::generateUV()
{
    m_uvGenerated = true;
    // Create a VBO on our GPU and store its handle in bufUV
    mp_context->glGenBuffers(1, &m_bufUV);
}

void Drawable::generatePos_sec()
{
    m_posGenerated_sec = true;
    // Create a VBO on our GPU and store its handle in bufPos_sec
    mp_context->glGenBuffers(1, &m_bufPos_sec);
}

void Drawable::generateNor_sec()
{
    m_norGenerated_sec = true;
    // Create a VBO on our GPU and store its handle in bufNor_sec
    mp_context->glGenBuffers(1, &m_bufNor_sec);
}

void Drawable::generateUV_sec()
{
    m_uvGenerated_sec = true;
    // Create a VBO on our GPU and store its handle in bufUV_sec
    mp_context->glGenBuffers(1, &m_bufUV_sec);
}


void Drawable::generateIdx_sec()
{
    m_idxGenerated_sec = true;
    // Create a VBO on our GPU and store its handle in bufIdx_sec
    mp_context->glGenBuffers(1, &m_bufIdx_sec);
}

bool Drawable::bindIdx()
{
    if(m_idxGenerated) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    }
    return m_idxGenerated;
}

bool Drawable::bindPos()
{
    if(m_posGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    }
    return m_posGenerated;
}

bool Drawable::bindNor()
{
    if(m_norGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufNor);
    }
    return m_norGenerated;
}

bool Drawable::bindCol()
{
    if(m_colGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufCol);
    }
    return m_colGenerated;
}


bool Drawable::bindUV()
{
    if(m_uvGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUV);
    }
    return m_uvGenerated;
}

bool Drawable::bindIdx_sec()
{
    if(m_idxGenerated_sec) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx_sec);
    }
    return m_idxGenerated_sec;
}

bool Drawable::bindPos_sec()
{
    if(m_posGenerated_sec){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos_sec);
    }
    return m_posGenerated_sec;
}

bool Drawable::bindNor_sec()
{
    if(m_norGenerated_sec){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufNor_sec);
    }
    return m_norGenerated_sec;
}

bool Drawable::bindUV_sec()
{
    if(m_uvGenerated_sec){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUV_sec);
    }
    return m_uvGenerated_sec;
}

InstancedDrawable::InstancedDrawable(OpenGLContext *context)
    : Drawable(context), m_numInstances(0), m_bufPosOffset(-1), m_offsetGenerated(false)
{}

InstancedDrawable::~InstancedDrawable(){}

int InstancedDrawable::instanceCount() const {
    return m_numInstances;
}

void InstancedDrawable::generateOffsetBuf() {
    m_offsetGenerated = true;
    mp_context->glGenBuffers(1, &m_bufPosOffset);
}

bool InstancedDrawable::bindOffsetBuf() {
    if(m_offsetGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPosOffset);
    }
    return m_offsetGenerated;
}


void InstancedDrawable::clearOffsetBuf() {
    if(m_offsetGenerated) {
        mp_context->glDeleteBuffers(1, &m_bufPosOffset);
        m_offsetGenerated = false;
    }
}
void InstancedDrawable::clearColorBuf() {
    if(m_colGenerated) {
        mp_context->glDeleteBuffers(1, &m_bufCol);
        m_colGenerated = false;
    }
}
