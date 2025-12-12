#pragma once
#include <iostream>
#include <memory>
#include <vector>
#include <set>
#include <string>
#include <cmath>

enum NpcType {
    Unknown = 0,
    DragonType = 1,
    BullType = 2,
    FrogType = 3
};

struct Dragon; struct Bull; struct Frog;

struct IFightObserver {
    virtual void on_fight(const std::shared_ptr<class NPC> attacker,
                          const std::shared_ptr<class NPC> defender,
                          bool win) = 0;
};

struct FightVisitor {
    virtual void visit(Dragon& d) = 0;
    virtual void visit(Bull& b) = 0;
    virtual void visit(Frog& f) = 0;
};

struct NPC : public std::enable_shared_from_this<NPC> {
public:
    NpcType type;
    std::string name;
    int x{0}, y{0};
    std::vector<std::shared_ptr<IFightObserver>> observers;
    
    bool alive = true;

    NPC(NpcType t, const std::string& n, int x, int y);
    NPC(NpcType t, std::istream& is);
    void subscribe(std::shared_ptr<IFightObserver> obs);
    void fight_notify(const std::shared_ptr<NPC> defender, bool win);
    bool is_close(const std::shared_ptr<NPC>& other, size_t dist) const;
    virtual ~NPC();
    virtual char get_symbol() const = 0;
    virtual void print() = 0;
    virtual void save(std::ostream& os);
    virtual void accept(std::shared_ptr<FightVisitor> v) = 0;
    friend std::ostream& operator<<(std::ostream& os, NPC& npc);
};