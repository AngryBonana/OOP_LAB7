#pragma once

#include "npc.h"

struct Bull : public NPC {

    Bull(const std::string& name, int x, int y);

    Bull(std::istream& is);

    void print() override;

    void save(std::ostream& os) override;

    void accept(std::shared_ptr<FightVisitor> v) override;

    friend std::ostream& operator<<(std::ostream& os, Bull& b);

    char get_symbol() const override;
};