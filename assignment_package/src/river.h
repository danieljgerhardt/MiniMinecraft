#ifndef RIVER_H
#define RIVER_H

#include "la.h"

enum RiverSymbolType {
    FL, //forward left
    FR, //forward right,
    CL, //curve left
    CR, //curve right
    SA, //save
    LD, //load

};

struct Turtle {
    glm::ivec4 pos;
    float xAmt;
    float zAmt;
    bool leaf;
};

class RiverSymbol {
private:
    static int topId;

    int id;
    RiverSymbolType sym;
public:
    RiverSymbol();
    RiverSymbol(RiverSymbolType sym);
    ~RiverSymbol();
    int getId() const;
    RiverSymbolType getSym() const;
    void setSym(RiverSymbolType sym);
};

struct Postcondition {
    float probability;
    std::vector<RiverSymbolType> new_symbols;
};

struct Rule {
    RiverSymbolType precondition;
    std::vector<Postcondition> pcs;
};

class RiverLSystem {
private:
    std::vector<RiverSymbol> axiom;
    std::vector<Rule> grammar;

public:
    RiverLSystem();
    RiverLSystem(std::vector<RiverSymbol>& axiom, std::vector<Rule>& grammar);
    ~RiverLSystem();

    std::vector<RiverSymbol> applyRandomRule(const RiverSymbol& old_symbol, const std::vector<Rule>& grammar);
    void replace(RiverSymbol& old_symbol, std::vector<RiverSymbol>& new_symbol, std::vector<RiverSymbol>& axiom);
    void parseLSystem(int iterations);
    std::vector<RiverSymbol>& getAxiom();
};

RiverLSystem makeRiverLSystem();

#endif // RIVER_H
