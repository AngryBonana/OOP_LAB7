#pragma once
#include "npc.h"
#include "dragon.h"
#include "bull.h"
#include "frog.h"

struct FightContext {
    std::shared_ptr<NPC> attacker;
    std::shared_ptr<NPC> defender;
    bool kill_defender = false;
};

struct FightVisitorImpl : public FightVisitor {
    FightContext& ctx;
    FightVisitorImpl(FightContext& c) : ctx(c) {}

    void visit(Dragon& d) override {
        ctx.kill_defender = false;
    }

    void visit(Bull& b) override {
        ctx.kill_defender = (ctx.attacker->type == DragonType);
    }

    void visit(Frog& f) override {
        ctx.kill_defender = (ctx.attacker->type == BullType);
    }
};