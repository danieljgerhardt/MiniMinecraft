#ifndef INVENTORY_H
#define INVENTORY_H

#include <array>
#include "scene/chunk.h"
#include "shaderprogram.h"

class Inventory;

class ItemFrame : public Drawable {
private:
    bool selected;
    int position;
public:
    ItemFrame(OpenGLContext* context, int position);

    void createVBOdata();

    void toggleSelected();
};

class Item : public Drawable {
private:
    BlockType type;
    int position;
public:
    Item(OpenGLContext* context, BlockType type, int position);

    void createVBOdata();

    BlockType getType();
};

class CrossHair : public Drawable {
public:
    CrossHair(OpenGLContext* context);
    void createVBOdata();
};

class Inventory
{
private:
    std::array<Item,10> items;
    std::array<ItemFrame, 10> frames;
    int selected;
    CrossHair crosshair;
public:
    Inventory(OpenGLContext* context);
    ~Inventory();

    void selectSlot(int num);
    BlockType getSelected();

    void draw(ShaderProgram* prog, int blockTexture, int slotTexture);

    void initalize();
    void destroy();
};



#endif // INVENTORY_H
