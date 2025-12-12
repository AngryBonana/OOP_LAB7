#include "../include/bull.h"


Bull::Bull(const std::string& name, int x, int y) : NPC(BullType, name, x, y) {}

Bull::Bull(std::istream& is) : NPC(BullType, is) {}

void Bull::print() { 
    std::cout << *this; 
}

void Bull::save(std::ostream& os) { 
    os << BullType << "\n"; NPC::save(os); 
}

void Bull::accept(std::shared_ptr<FightVisitor> v) { 
    v->visit(*this); 
}

std::ostream& operator<<(std::ostream& os, Bull& b) {
    os << "bull " << static_cast<NPC&>(b) << "\n";
    return os;
}

char Bull::get_symbol() const { 
    return 'B'; 
}