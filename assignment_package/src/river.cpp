#include "river.h"
#include <random>

int RiverSymbol::topId = -1;

RiverSymbol::RiverSymbol() : id(topId + 1) {
    topId++;
}

RiverSymbol::RiverSymbol(RiverSymbolType sym) : id(topId + 1), sym(sym) {
    topId++;
}

RiverSymbol::~RiverSymbol() {}

int RiverSymbol::getId() const {
    return this->id;
}

RiverSymbolType RiverSymbol::getSym() const {
    return this->sym;
}

void RiverSymbol::setSym(RiverSymbolType sym) {
    this->sym = sym;
}

RiverLSystem::RiverLSystem() : axiom(), grammar() {}

RiverLSystem::RiverLSystem(std::vector<RiverSymbol>& axiom, std::vector<Rule>& grammar) : axiom(axiom), grammar(grammar) {}

RiverLSystem::~RiverLSystem() {}

std::vector<RiverSymbol> RiverLSystem::applyRandomRule(const RiverSymbol& old_symbol, const std::vector<Rule>& grammar) {
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
                    std::vector<RiverSymbol> ret;
                    for (RiverSymbolType s : pc.new_symbols) {
                        ret.push_back(RiverSymbol(s));
                    }
                    return ret;
                }
            }
        }
    }
    return {old_symbol};
}

void RiverLSystem::replace(RiverSymbol& old_symbol, std::vector<RiverSymbol>& new_symbol, std::vector<RiverSymbol>& axiom) {
    for (int i = 0; i < (int)axiom.size(); ++i) {
        if (axiom.at(i).getId() == old_symbol.getId()) {
            axiom.insert(axiom.begin() + i, new_symbol.begin(), new_symbol.end());
            return;
        }
    }
}

void RiverLSystem::parseLSystem(int iterations) {
    std::vector<RiverSymbol> result = axiom;

    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<RiverSymbol> newAxiom;
        for (size_t i = 0; i < result.size(); ++i) {
            RiverSymbol old_symbol = result[i];
            std::vector<RiverSymbol> new_symbols = applyRandomRule(old_symbol, grammar);

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

std::vector<RiverSymbol>& RiverLSystem::getAxiom() {
    return this->axiom;
}

RiverLSystem makeRiverLSystem() {

}
