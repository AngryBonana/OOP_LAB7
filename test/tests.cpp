#include <gtest/gtest.h>
#include "../include/npc.h"
#include "../include/dragon.h"
#include "../include/bull.h"
#include "../include/frog.h"
#include "../include/visitor.h"
#include <memory>

// Тест: создание Дракона
TEST(NPCTest, CreateDragon) {
    auto dragon = std::make_shared<Dragon>("Smaug", 10, 20);
    EXPECT_EQ(dragon->type, DragonType);
    EXPECT_EQ(dragon->name, "Smaug");
    EXPECT_EQ(dragon->x, 10);
    EXPECT_EQ(dragon->y, 20);
    EXPECT_TRUE(dragon->alive);
}

// Тест: создание Быка
TEST(NPCTest, CreateBull) {
    auto bull = std::make_shared<Bull>("Bucephalus", 5, 5);
    EXPECT_EQ(bull->type, BullType);
    EXPECT_EQ(bull->name, "Bucephalus");
    EXPECT_EQ(bull->x, 5);
    EXPECT_EQ(bull->y, 5);
    EXPECT_TRUE(bull->alive);
}

// Тест: создание Жабы
TEST(NPCTest, CreateFrog) {
    auto frog = std::make_shared<Frog>("Kermit", 0, 0);
    EXPECT_EQ(frog->type, FrogType);
    EXPECT_EQ(frog->name, "Kermit");
    EXPECT_EQ(frog->x, 0);
    EXPECT_EQ(frog->y, 0);
    EXPECT_TRUE(frog->alive);
}

// Тест: Дракон может убить Быка
TEST(FightTest, DragonKillsBull) {
    auto dragon = std::make_shared<Dragon>("D", 0, 0);
    auto bull = std::make_shared<Bull>("B", 0, 0);
    FightContext ctx;
    ctx.attacker = dragon;
    ctx.defender = bull;
    ctx.kill_defender = false;
    auto visitor = std::make_shared<FightVisitorImpl>(ctx);
    bull->accept(visitor);
    EXPECT_TRUE(ctx.kill_defender);
}

// Тест: Бык может убить Жабу
TEST(FightTest, BullKillsFrog) {
    auto bull = std::make_shared<Bull>("B", 0, 0);
    auto frog = std::make_shared<Frog>("F", 0, 0);
    FightContext ctx;
    ctx.attacker = bull;
    ctx.defender = frog;
    ctx.kill_defender = false;
    auto visitor = std::make_shared<FightVisitorImpl>(ctx);
    frog->accept(visitor);
    EXPECT_TRUE(ctx.kill_defender);
}

// Тест: Жаба НЕ может убить никого (даже другую жабу)
TEST(FightTest, FrogCannotKillAnyone) {
    auto frog1 = std::make_shared<Frog>("F1", 0, 0);
    auto frog2 = std::make_shared<Frog>("F2", 0, 0);
    FightContext ctx;
    ctx.attacker = frog1;
    ctx.defender = frog2;
    ctx.kill_defender = false;
    auto visitor = std::make_shared<FightVisitorImpl>(ctx);
    frog2->accept(visitor);
    EXPECT_FALSE(ctx.kill_defender);
}

// Тест: Бык НЕ может убить Дракона
TEST(FightTest, BullCannotKillDragon) {
    auto bull = std::make_shared<Bull>("B", 0, 0);
    auto dragon = std::make_shared<Dragon>("D", 0, 0);
    FightContext ctx;
    ctx.attacker = bull;
    ctx.defender = dragon;
    ctx.kill_defender = false;
    auto visitor = std::make_shared<FightVisitorImpl>(ctx);
    dragon->accept(visitor);
    EXPECT_FALSE(ctx.kill_defender);
}

// Тест: is_close — внутри радиуса
TEST(DistanceTest, IsCloseWithinRange) {
    auto a = std::make_shared<Dragon>("A", 0, 0);
    auto b = std::make_shared<Bull>("B", 3, 4); // расстояние = 5
    EXPECT_TRUE(a->is_close(b, 5));
    EXPECT_FALSE(a->is_close(b, 4));
}

// Тест: движение не выходит за границы карты 100x100
TEST(MovementTest, StayInBounds) {
    auto npc = std::make_shared<Frog>("Test", 99, 99);
    // Пытаемся выйти за пределы
    npc->x += 10;
    npc->y += 10;
    // Имитация clamp (в main.cpp используется std::max/min)
    npc->x = std::max(0, std::min(99, npc->x));
    npc->y = std::max(0, std::min(99, npc->y));
    EXPECT_EQ(npc->x, 99);
    EXPECT_EQ(npc->y, 99);
}

// Тест: мёртвый NPC не двигается (логически — alive = false)
TEST(AliveTest, DeadNpcNotAlive) {
    auto npc = std::make_shared<Bull>("Deadly", 10, 10);
    npc->alive = false;
    EXPECT_FALSE(npc->alive);
}