#ifndef TREE_H
#define TREE_H

#include "la.h"

enum TreeSymbolType {
    T, //place tree block, move turtle in direction of placement
    TB, //tree base, base of tree and won't expand
    B, //make branch
    L, //leaf
    LT, //top leaf
    PX, //rotate positive x
    NX, //rotate negative y
    PZ, //rotate positive z
    NZ, //rotate negative z
    SA, //[, save
    LD //], store
};

struct Turtle {
    glm::vec4 pos;
    float xAmt;
    float zAmt;
    bool leaf;
};

class TreeSymbol {
private:
    static int topId;

    int id;
    TreeSymbolType sym;
public:
    TreeSymbol();
    TreeSymbol(TreeSymbolType sym);
    ~TreeSymbol();
    int getId() const;
    TreeSymbolType getSym() const;
    void setSym(TreeSymbolType sym);
};

struct Postcondition {
    float probability;
    std::vector<TreeSymbolType> new_symbols;
};

struct Rule {
    TreeSymbolType precondition;
    std::vector<Postcondition> pcs;
};

class TreeLSystem {
private:
    std::vector<TreeSymbol> axiom;
    std::vector<Rule> grammar;

public:
    TreeLSystem();
    TreeLSystem(std::vector<TreeSymbol>& axiom, std::vector<Rule>& grammar);
    ~TreeLSystem();
    
    std::vector<TreeSymbol> applyRandomRule(const TreeSymbol& old_symbol, const std::vector<Rule>& grammar);
    void replace(TreeSymbol& old_symbol, std::vector<TreeSymbol>& new_symbol, std::vector<TreeSymbol>& axiom);
    void parseLSystem(int iterations);
    std::vector<TreeSymbol>& getAxiom();
};

TreeLSystem makeTreeLSystem();

class Tree {
public:
    Tree();
    Tree(glm::ivec4 pos, int startingStage);

    void performIteration();
    void placeTree();
    std::vector<TreeSymbol>& getAxiom();
    bool getCanGrow() const;
    glm::ivec4& getPos();
    qint64 getOffset() const;
private:
    int stage;
    bool canGrow;
    glm::ivec4 pos;
    TreeLSystem system;
    qint64 timeOffset;
};

#endif // TREE_H
