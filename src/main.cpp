#include "../include/npc.h"
#include "../include/dragon.h"
#include "../include/bull.h"
#include "../include/frog.h"
#include "../include/visitor.h"

#include <fstream>
#include <random>
#include <thread>
#include <queue>
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <vector>

constexpr int MAP_SIZE = 100;
constexpr int NPC_COUNT = 50;
constexpr int GAME_DURATION_SEC = 30;

std::shared_mutex npcs_mutex;
std::mutex cout_mutex;
std::mutex fight_queue_mutex;

struct NpcStats { int move, kill; };
const std::unordered_map<NpcType, NpcStats> stats = {
    {DragonType, {50, 30}},
    {BullType,   {30, 10}},
    {FrogType,   {1,  10}}
};

std::queue<std::pair<std::shared_ptr<NPC>, std::shared_ptr<NPC>>> fight_queue;

class TextObserver : public IFightObserver {
    static TextObserver inst;
public:
    static std::shared_ptr<IFightObserver> get() {
        return std::shared_ptr<IFightObserver>(&inst, [](auto*){});
    }
    void on_fight(const std::shared_ptr<NPC> a, const std::shared_ptr<NPC> d, bool win) override {
        if (win) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "[FIGHT] " << a->name << " killed " << d->name << "\n";
        }
    }
};
TextObserver TextObserver::inst;

class FileObserver : public IFightObserver {
    std::ofstream log{"log.txt"};
public:
    void on_fight(const std::shared_ptr<NPC> a, const std::shared_ptr<NPC> d, bool win) override {
        if (win) {
            log << "Murder: " << a->name << " killed " << d->name << "\n";
            log.flush();
        }
    }
};

std::shared_ptr<NPC> create_npc(NpcType t, const std::string& name, int x, int y) {
    std::shared_ptr<NPC> npc;
    switch (t) {
        case DragonType: npc = std::make_shared<Dragon>(name, x, y); break;
        case BullType:   npc = std::make_shared<Bull>(name, x, y); break;
        case FrogType:   npc = std::make_shared<Frog>(name, x, y); break;
        default: return nullptr;
    }
    static auto file_obs = std::make_shared<FileObserver>();
    npc->subscribe(TextObserver::get());
    npc->subscribe(file_obs);
    return npc;
}

std::vector<std::shared_ptr<NPC>> generate_npcs() {
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> type(1, 3), coord(0, MAP_SIZE - 1);
    std::vector<std::shared_ptr<NPC>> npcs;
    for (int i = 0; i < NPC_COUNT; ++i) {
        npcs.push_back(create_npc(static_cast<NpcType>(type(gen)),
                                  "NPC_" + std::to_string(i),
                                  coord(gen), coord(gen)));
    }
    return npcs;
}

void print_map(const std::vector<std::shared_ptr<NPC>>& npcs) {
    std::vector<std::vector<char>> grid(MAP_SIZE, std::vector<char>(MAP_SIZE, '.'));
    {
        std::shared_lock<std::shared_mutex> lock(npcs_mutex);
        for (const auto& n : npcs)
            if (n->alive) {
                int x = std::clamp(n->x, 0, MAP_SIZE - 1);
                int y = std::clamp(n->y, 0, MAP_SIZE - 1);
                grid[y][x] = n->get_symbol();
            }
    }
    for (const auto& row : grid) {
        for (char c : row) std::cout << c;
        std::cout << "\n";
    }
}

void move_thread(std::atomic<bool>& stop, std::vector<std::shared_ptr<NPC>>& npcs) {
    std::mt19937 gen(std::random_device{}());
    while (!stop) {
        std::shared_lock<std::shared_mutex> lock(npcs_mutex);
        for (auto& n : npcs) {
            if (!n->alive) continue;
            auto& s = stats.at(n->type);
            std::uniform_int_distribution<> dx(-s.move, s.move), dy(-s.move, s.move);
            n->x = std::clamp(n->x + dx(gen), 0, MAP_SIZE - 1);
            n->y = std::clamp(n->y + dy(gen), 0, MAP_SIZE - 1);
            for (auto& other : npcs) {
                if (other != n && other->alive && n->is_close(other, s.kill)) {
                    std::lock_guard<std::mutex> q(fight_queue_mutex);
                    fight_queue.emplace(n, other);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void fight_thread(std::atomic<bool>& stop) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> die(1, 6);
    while (!stop) {
        std::pair<std::shared_ptr<NPC>, std::shared_ptr<NPC>> p;
        {
            std::lock_guard<std::mutex> q(fight_queue_mutex);
            if (fight_queue.empty()) { std::this_thread::sleep_for(std::chrono::milliseconds(50));; continue; }
            p = fight_queue.front(); fight_queue.pop();
        }
        auto [a, d] = p;
        if (!a->alive || !d->alive) continue;
        FightContext ctx{a, d, false};
        d->accept(std::make_shared<FightVisitorImpl>(ctx));
        if (!ctx.kill_defender) continue;
        if (die(gen) > die(gen)) d->alive = false;
        a->fight_notify(d, true);
    }
}

int main() {
    auto npcs = generate_npcs();
    std::atomic<bool> stop{false};
    std::thread t1(move_thread, std::ref(stop), std::ref(npcs));
    std::thread t2(fight_thread, std::ref(stop));

    for (int sec = 1; sec <= GAME_DURATION_SEC; ++sec) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\n--- FRAME " << sec << " ---\n";
        print_map(npcs);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    stop = true;
    t1.join(); t2.join();

    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "\n=== SURVIVORS ===\n";
    std::shared_lock<std::shared_mutex> lock2(npcs_mutex);
    for (auto& n : npcs)
        if (n->alive) std::cout << n->name << " (" << n->get_symbol() << ") at (" << n->x << "," << n->y << ")\n";

    return 0;
}