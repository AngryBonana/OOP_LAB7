#include "../include/dragon.h"


Dragon::Dragon(const std::string& name, int x, int y) : NPC(DragonType, name, x, y) {}

Dragon::Dragon(std::istream& is) : NPC(DragonType, is) {}

void Dragon::print() { 
    std::cout << *this; 
}

void Dragon::save(std::ostream& os) { 
    os << DragonType << "\n"; NPC::save(os); 
}

void Dragon::accept(std::shared_ptr<FightVisitor> v) { 
    v->visit(*this); 
}

std::ostream& operator<<(std::ostream& os, Dragon& d) {
    os << "dragon " << static_cast<NPC&>(d) << "\n";
    return os;
}
char Dragon::get_symbol() const { 
    return 'D'; 
}