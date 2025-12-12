#pragma once

#include "npc.h"

struct Frog : public NPC {

    Frog(const std::string& name, int x, int y);

    Frog(std::istream& is);

    void print() override;

    void save(std::ostream& os) override;

    void accept(std::shared_ptr<FightVisitor> v) override;

    friend std::ostream& operator<<(std::ostream& os, Frog& f);

    char get_symbol() const override;
};