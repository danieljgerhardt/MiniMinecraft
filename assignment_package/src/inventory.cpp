#include "inventory.h"

ItemFrame::ItemFrame(OpenGLContext* context, int position)
    : Drawable(context), selected(false), position(position) {

}

void ItemFrame::createVBOdata() {
    GLuint idx[6]{0, 1, 2, 0, 2, 3};
    glm::vec4 vert_pos[4] {glm::vec4(-0.9f + (position * 0.15), -0.9f, 0.99f, 1.f),
                          glm::vec4(-0.75f + (position * 0.15), -0.9f, 0.99f, 1.f),
                          glm::vec4(-0.75f + (position * 0.15), -0.75, 0.99f, 1.f),
                          glm::vec4(-0.9f + (position * 0.15), -0.75f, 0.99f, 1.f)};

    std::array<glm::vec2,4> vert_UV;

    if (!selected) {
        vert_UV = {glm::vec2(0.5f, 0.f),
            glm::vec2(1.f, 0.f),
            glm::vec2(1.0f, 0.5f),
            glm::vec2(0.5f, 0.5f)};
    } else {
        vert_UV = {glm::vec2(0.f, 0.f),
            glm::vec2(0.5f, 0.f),
            glm::vec2(.5f, 0.5f),
            glm::vec2(0.f, 0.5f)};
    }


    m_count = 6;

    // Create a VBO on our GPU and store its handle in bufIdx
    generateIdx();
    // Tell OpenGL that we want to perform subsequent operations on the VBO referred to by bufIdx
    // and that it will be treated as an element array buffer (since it will contain triangle indices)
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    // Pass the data stored in cyl_idx into the bound buffer, reading a number of bytes equal to
    // CYL_IDX_COUNT multiplied by the size of a GLuint. This data is sent to the GPU to be read by shader programs.
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), idx, GL_STATIC_DRAW);

    // The next few sets of function calls are basically the same as above, except bufPos and bufNor are
    // array buffers rather than element array buffers, as they store vertex attributes like position.
    generatePos();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), vert_pos, GL_STATIC_DRAW);
    generateUV();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUV);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), vert_UV.data(), GL_STATIC_DRAW);
}

void ItemFrame::toggleSelected() {
    selected = !selected;
    destroyVBOdata();
    createVBOdata();
}

Item::Item(OpenGLContext* context, BlockType type, int position)
    : Drawable(context), type(type), position(position) {

}

void Item::createVBOdata() {
    GLuint idx[6]{0, 1, 2, 0, 2, 3};
    glm::vec4 vert_pos[4] {glm::vec4(-0.88f + (position * 0.15), -0.88f, 0.99f, 1.f),
                          glm::vec4(-0.77f + (position * 0.15), -0.88f, 0.99f, 1.f),
                          glm::vec4(-0.77f + (position * 0.15), -0.77, 0.99f, 1.f),
                          glm::vec4(-0.88f + (position * 0.15), -0.77f, 0.99f, 1.f)};

    glm::vec2 bottomRight = 0.0625f * block_info_map.at(type).uv_map.at(XPOS);

    glm::vec2 vert_UV[4] {bottomRight + glm::vec2(0.f, 0.f),
                         bottomRight + glm::vec2(0.0625f, 0.f),
                         bottomRight + glm::vec2(0.0625f, 0.0625f),
                         bottomRight + glm::vec2(0.f, 0.0625f)};

    m_count = 6;

    // Create a VBO on our GPU and store its handle in bufIdx
    generateIdx();
    // Tell OpenGL that we want to perform subsequent operations on the VBO referred to by bufIdx
    // and that it will be treated as an element array buffer (since it will contain triangle indices)
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    // Pass the data stored in cyl_idx into the bound buffer, reading a number of bytes equal to
    // CYL_IDX_COUNT multiplied by the size of a GLuint. This data is sent to the GPU to be read by shader programs.
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), idx, GL_STATIC_DRAW);

    // The next few sets of function calls are basically the same as above, except bufPos and bufNor are
    // array buffers rather than element array buffers, as they store vertex attributes like position.
    generatePos();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), vert_pos, GL_STATIC_DRAW);
    generateUV();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUV);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), vert_UV, GL_STATIC_DRAW);
}

BlockType Item::getType() {
    return type;
}

Inventory::Inventory(OpenGLContext* context)
    : items({Item(context,GRASS,1),
            Item(context,DIRT,2),
            Item(context,STONE,3),
            Item(context,SNOW,4),
            Item(context,GLASS,5),
            Item(context,WATER,6),
            Item(context,LAVA,7),
            Item(context,OAK_LOG,8),
            Item(context,OAK_LEAVES,9),
             Item(context,BONE,10)}),
    frames({ItemFrame(context,1),
             ItemFrame(context,2),
             ItemFrame(context,3),
             ItemFrame(context,4),
             ItemFrame(context,5),
             ItemFrame(context,6),
             ItemFrame(context,7),
             ItemFrame(context,8),
             ItemFrame(context,9),
              ItemFrame(context,10)}), selected(0), crosshair(context) {

}

void Inventory::selectSlot(int num) {
    frames[selected].toggleSelected();
    selected = num - 1;
    frames[selected].toggleSelected();
}

BlockType Inventory::getSelected() {
    return items[selected].getType();
}

void Inventory::draw(ShaderProgram* prog, int blockTexture, int slotTexture) {
    glDisable(GL_DEPTH_TEST);
    prog->setTexture(slotTexture);
    for(ItemFrame& f : frames) {
        prog->draw(f);
    }
    prog->draw(crosshair);
    prog->setTexture(blockTexture);
    for(Item& i : items) {
        prog->draw(i);
    }

    glEnable(GL_DEPTH_TEST);
}

void Inventory::initalize() {
    for(ItemFrame& f : frames) {
        f.createVBOdata();
    }
    for(Item& i : items) {
        i.createVBOdata();
    }
    crosshair.createVBOdata();

    frames[0].toggleSelected();
}

void Inventory::destroy() {
    for(ItemFrame& f : frames) {
        f.destroyVBOdata();
    }
    for(Item& i : items) {
        i.destroyVBOdata();
    }
    crosshair.destroyVBOdata();
}

Inventory::~Inventory() {
    destroy();
}

void CrossHair::createVBOdata() {
    GLuint idx[6]{0, 1, 2, 0, 2, 3};
    glm::vec4 vert_pos[4] {glm::vec4(-0.05f, -0.05f, 0.99f, 1.f),
                          glm::vec4(0.05f, -0.05f, 0.99f, 1.f),
                          glm::vec4(0.05f, 0.05f, 0.99f, 1.f),
                          glm::vec4(-0.05f, 0.05f, 0.99f, 1.f)};

    glm::vec2 vert_UV[4] {glm::vec2(0.f, 0.5f),
                         glm::vec2(0.5f, 0.5f),
                         glm::vec2(0.5f, 1.f),
                         glm::vec2(0.f, 1.f)};

    m_count = 6;

    // Create a VBO on our GPU and store its handle in bufIdx
    generateIdx();
    // Tell OpenGL that we want to perform subsequent operations on the VBO referred to by bufIdx
    // and that it will be treated as an element array buffer (since it will contain triangle indices)
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    // Pass the data stored in cyl_idx into the bound buffer, reading a number of bytes equal to
    // CYL_IDX_COUNT multiplied by the size of a GLuint. This data is sent to the GPU to be read by shader programs.
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), idx, GL_STATIC_DRAW);

    // The next few sets of function calls are basically the same as above, except bufPos and bufNor are
    // array buffers rather than element array buffers, as they store vertex attributes like position.
    generatePos();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec4), vert_pos, GL_STATIC_DRAW);
    generateUV();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUV);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), vert_UV, GL_STATIC_DRAW);
}

CrossHair::CrossHair(OpenGLContext* context)
    : Drawable(context) {

}
