#include "entitydrawable.h"
#include <la.h>

EntityDrawable::EntityDrawable(OpenGLContext* context)
    : count(-1), bufIdx(), bufPos(), bufNor(), bufCol(),
    idxBound(false), posBound(false), norBound(false), colBound(false),
    mp_context(context)
{}

EntityDrawable::~EntityDrawable() {
    destroy();
}


void EntityDrawable::destroy()
{
    mp_context->glDeleteBuffers(1, &bufIdx);
    mp_context->glDeleteBuffers(1, &bufPos);
    mp_context->glDeleteBuffers(1, &bufNor);
    mp_context->glDeleteBuffers(1, &bufCol);
    mp_context->glDeleteBuffers(1, &bufWeights);
    mp_context->glDeleteBuffers(1, &bufIds);
}

GLenum EntityDrawable::drawMode()
{
    // Since we want every three indices in bufIdx to be
    // read to draw our Drawable, we tell that the draw mode
    // of this Drawable is GL_TRIANGLES

    // If we wanted to draw a wireframe, we would return GL_LINES

    return GL_TRIANGLES;
}

int EntityDrawable::elemCount()
{
    return count;
}

void EntityDrawable::generateIdx()
{
    idxBound = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &bufIdx);
}

void EntityDrawable::generatePos()
{
    posBound = true;
    // Create a VBO on our GPU and store its handle in bufPos
    mp_context->glGenBuffers(1, &bufPos);
}

void EntityDrawable::generateNor()
{
    norBound = true;
    // Create a VBO on our GPU and store its handle in bufNor
    mp_context->glGenBuffers(1, &bufNor);
}

void EntityDrawable::generateCol()
{
    colBound = true;
    // Create a VBO on our GPU and store its handle in bufCol
    mp_context->glGenBuffers(1, &bufCol);
}

void EntityDrawable::generateIds()
{
    idsBound = true;
    mp_context->glGenBuffers(1, &bufIds);
}

void EntityDrawable::generateWeights()
{
    weightsBound = true;
    mp_context->glGenBuffers(1, &bufWeights);
}

bool EntityDrawable::bindIdx()
{
    if(idxBound) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdx);
    }
    return idxBound;
}

bool EntityDrawable::bindPos()
{
    if(posBound){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufPos);
    }
    return posBound;
}

bool EntityDrawable::bindNor()
{
    if(norBound){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufNor);
    }
    return norBound;
}

bool EntityDrawable::bindCol()
{
    if(colBound){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufCol);
    }
    return colBound;
}

bool EntityDrawable::bindIds()
{
    if (idsBound) {
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufIds);
    }
    return idsBound;
}

bool EntityDrawable::bindWeights()
{
    if (weightsBound) {
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufWeights);
    }
    return weightsBound;
}
