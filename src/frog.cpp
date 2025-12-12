#include "../include/frog.h"


Frog::Frog(const std::string& name, int x, int y) : NPC(FrogType, name, x, y) {}

Frog::Frog(std::istream& is) : NPC(FrogType, is) {}

void Frog::print() { 
    std::cout << *this; }
    

void Frog::save(std::ostream& os) { 
    os << FrogType << "\n"; NPC::save(os); 
}

void Frog::accept(std::shared_ptr<FightVisitor> v) { 
    v->visit(*this); 
}

std::ostream& operator<<(std::ostream& os, Frog& f) {
    os << "frog " << static_cast<NPC&>(f) << "\n";
    return os;
}

char Frog::get_symbol() const {
    return 'F'; 
}