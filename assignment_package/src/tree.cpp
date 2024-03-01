#include "tree.h"
#include <random>

int TreeSymbol::topId = -1;

TreeSymbol::TreeSymbol() : id(topId + 1) {
    topId++;
}

TreeSymbol::TreeSymbol(TreeSymbolType sym) : id(topId + 1), sym(sym) {
    topId++;
}

TreeSymbol::~TreeSymbol() {}

int TreeSymbol::getId() const {
    return this->id;
}

TreeSymbolType TreeSymbol::getSym() const {
    return this->sym;
}

void TreeSymbol::setSym(TreeSymbolType sym) {
    this->sym = sym;
}

TreeLSystem::TreeLSystem() : axiom(), grammar() {}

TreeLSystem::TreeLSystem(std::vector<TreeSymbol>& axiom, std::vector<Rule>& grammar) : axiom(axiom), grammar(grammar) {}

TreeLSystem::~TreeLSystem() {}

std::vector<TreeSymbol> TreeLSystem::applyRandomRule(const TreeSymbol& old_symbol, const std::vector<Rule>& grammar) {
    for (const Rule& rule : grammar) {
        if (rule.precondition == old_symbol.getSym()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);
            double random_value = dis(gen);

            double cumulative_probability = 0.0;
            for (const Postcondition& pc : rule.pcs) {
                cumulative_probability += pc.probability;
                if (random_value <= cumulative_probability) {
                    std::vector<TreeSymbol> ret;
                    for (TreeSymbolType s : pc.new_symbols) {
                        ret.push_back(TreeSymbol(s));
                    }
                    return ret;
                }
            }
        }
    }
    return {old_symbol};
}

void TreeLSystem::replace(TreeSymbol& old_symbol, std::vector<TreeSymbol>& new_symbol, std::vector<TreeSymbol>& axiom) {
    for (int i = 0; i < (int)axiom.size(); ++i) {
        if (axiom.at(i).getId() == old_symbol.getId()) {
            axiom.insert(axiom.begin() + i, new_symbol.begin(), new_symbol.end());
            return;
        }
    }
}

void TreeLSystem::parseLSystem(int iterations) {
    std::vector<TreeSymbol> result = axiom;

    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<TreeSymbol> newAxiom;
        for (size_t i = 0; i < result.size(); ++i) {
            TreeSymbol old_symbol = result[i];
            std::vector<TreeSymbol> new_symbols = applyRandomRule(old_symbol, grammar);

            // Remove the old symbol at index i
            result.erase(result.begin() + i);

            // Insert the new symbols at the position of the old symbol
            result.insert(result.begin() + i, new_symbols.begin(), new_symbols.end());

            // Update i to point to the last inserted symbol
            i += new_symbols.size() - 1;
        }
    }

    axiom = result;
}

std::vector<TreeSymbol>& TreeLSystem::getAxiom() {
    return this->axiom;
}

TreeLSystem makeTreeLSystem() {
    std::vector<TreeSymbol> axiom = {TB, TB, T, B};

    //Rule r1 -- branches
    float prob = 1.f / 7.f;
    Rule r1;
    r1.precondition = B;
    Postcondition r1pc1;
    r1pc1.probability = prob;
    r1pc1.new_symbols = {SA, NX, T, L, T, L, LT, B, LD,
                         SA, PX, T, L, T, L, T, LT, B, LD,
                         SA, PZ, T, L, T, L, T, L, LT, B, LD,
                         SA, NZ, T, L, T, L, LT, B, LD, T, L, T, B, T, B};
    Postcondition r1pc2;
    r1pc2.new_symbols = {SA, PZ, PX, T, T, LD, SA, NZ, NX, L, T, LT, B, LD};
    r1pc2.probability = prob;
    Postcondition r1pc3;
    r1pc3.new_symbols = {SA, NZ, NX, T, T, LD, SA, NZ, NX, L, T, LT, B, LD};
    r1pc3.probability = prob;
    Postcondition r1pc4;
    r1pc4.new_symbols = {SA, PZ, T, T, LT, B, LD, NX, T, L, T, LT, B};
    r1pc4.probability = prob;
    Postcondition r1pc5;
    r1pc5.new_symbols = {T, T, SA, PZ, NX, T, T, L, LD, B, T, SA, NZ, PX, T, T, L, LD, B, T, T, B, LT};
    r1pc5.probability = prob;
    Postcondition r1pc6;
    r1pc6.probability = prob;
    r1pc6.new_symbols = {SA, NX, T, L, T, L, LT, B, LD,
                         SA, PX, T, L, T, L, T, LT, B, LD,
                         SA, PZ, T, L, T, L, LT, B, LD, T, L, T, B, T, B};
    Postcondition r1pc7;
    r1pc7.probability = prob;
    r1pc7.new_symbols = {SA, PZ, T, LD, SA, PX, T, LD, SA, NZ, T, LD, SA, NX, T, B, LD, T, T, B};
    r1.pcs = {r1pc1, r1pc2, r1pc3, r1pc4, r1pc5, r1pc6, r1pc7};

    //Rule r2 -- tree height expansion
    Rule r2;
    r2.precondition = T;
    Postcondition r2pc1;
    r2pc1.probability = 0.1f;
    r2pc1.new_symbols = {T, B};
    Postcondition r2pc2;
    r2pc2.probability = 0.05f;
    r2pc2.new_symbols = {T, T};
    r2.pcs = {r2pc1, r2pc2};

    //Rule r3 -- expand tree base
    Rule r3;
    r3.precondition = TB;
    Postcondition r3pc1;
    r3pc1.probability = 0.5f;
    r3pc1.new_symbols = {TB, T};
    r3.pcs = {r3pc1};

    //Rule r4 -- remove extra LTs
    Rule r4;
    r4.precondition = LT;
    Postcondition r4pc1;
    r4pc1.probability = 1;
    r4pc1.new_symbols = {};
    r4.pcs = {r4pc1};

    std::vector<Rule> grammar = {r1, r2, r3, r4};
    return TreeLSystem(axiom, grammar);
}

Tree::Tree() : stage(0), canGrow(true), pos(glm::ivec4()), timeOffset() {
    system = makeTreeLSystem();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(7500, 12500);
    timeOffset = distribution(gen);
    this->system.parseLSystem(stage);
}

Tree::Tree(glm::ivec4 pos, int startingStage) : stage(startingStage), canGrow(true), pos(pos), timeOffset() {
    system = makeTreeLSystem();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(7500, 12500);
    timeOffset = distribution(gen);
    this->system.parseLSystem(stage);
}

std::vector<TreeSymbol>& Tree::getAxiom() {
    return this->system.getAxiom();
}

bool Tree::getCanGrow() const {
    return this->canGrow;
}

glm::ivec4& Tree::getPos() {
    return this->pos;
}

qint64 Tree::getOffset() const {
    return this->timeOffset;
}

void Tree::performIteration() {
    if (this->canGrow) {
        this->system.parseLSystem(1);
        this->stage++;
    }
    if (stage > 5) this->canGrow = false;
}
