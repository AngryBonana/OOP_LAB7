#pragma once
#include "npc.h"
struct Dragon : public NPC {

    Dragon(const std::string& name, int x, int y);
    
    Dragon(std::istream& is);
    
    void print() override;
    
    void save(std::ostream& os) override;
    
    void accept(std::shared_ptr<FightVisitor> v) override;
    
    friend std::ostream& operator<<(std::ostream& os, Dragon& d);

    char get_symbol() const override;
};