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
#include <iostream>


constexpr int MAP_SIZE = 100;
constexpr int NPC_COUNT = 50;
constexpr int GAME_DURATION_SEC = 30;


std::shared_mutex npcs_mutex;
std::mutex cout_mutex;
std::mutex fight_queue_mutex;


struct NpcStats {
    int move_distance;
    int kill_distance;
};

const std::unordered_map<NpcType, NpcStats> npc_stats = {
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
        }
    }
};
TextObserver TextObserver::inst;

class FileObserver : public IFightObserver {
    std::ofstream log{"log.txt"};
public:
    void on_fight(const std::shared_ptr<NPC> a, const std::shared_ptr<NPC> d, bool win) override {
    if (win) {
        auto type_to_str = [](NpcType t) -> const char* {
            switch (t) {
                case DragonType: return "Dragon";
                case BullType:   return "Bull";
                case FrogType:   return "Frog";
                default:         return "Unknown";
            }
        };
        log << "Murder: " 
            << type_to_str(a->type) << " (" << a->name << ") "
            << "killed " 
            << type_to_str(d->type) << " (" << d->name << ")\n";
        log.flush();
    }
}
};


std::shared_ptr<NPC> create_npc(NpcType type, const std::string& name, int x, int y) {
    std::shared_ptr<NPC> npc;
    switch (type) {
        case DragonType: npc = std::make_shared<Dragon>(name, x, y); break;
        case BullType:   npc = std::make_shared<Bull>(name, x, y); break;
        case FrogType:   npc = std::make_shared<Frog>(name, x, y); break;
        default: return nullptr;
    }
    if (npc) {
        npc->subscribe(TextObserver::get());
        static auto file_obs = std::make_shared<FileObserver>();
        npc->subscribe(file_obs);
    }
    return npc;
}


std::vector<std::shared_ptr<NPC>> generate_npcs() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> type_dist(1, 3);
    std::uniform_int_distribution<int> coord_dist(0, MAP_SIZE - 1);

    std::vector<std::shared_ptr<NPC>> npcs;
    for (int i = 0; i < NPC_COUNT; ++i) {
        NpcType t = static_cast<NpcType>(type_dist(gen));
        std::string name = "NPC_" + std::to_string(i);
        int x = coord_dist(gen);
        int y = coord_dist(gen);
        npcs.push_back(create_npc(t, name, x, y));
    }
    return npcs;
}


void print_map(const std::vector<std::shared_ptr<NPC>>& npcs) {
    std::vector<std::vector<char>> grid(MAP_SIZE, std::vector<char>(MAP_SIZE, '.'));
    {
        std::shared_lock<std::shared_mutex> lock(npcs_mutex);
        for (const auto& n : npcs) {
            if (n->alive.load()) {
                int x = std::clamp(n->x, 0, MAP_SIZE - 1);
                int y = std::clamp(n->y, 0, MAP_SIZE - 1);
                grid[y][x] = n->get_symbol();
            }
        }
    }
    for (int y = 0; y < MAP_SIZE; ++y) {
        for (int x = 0; x < MAP_SIZE; ++x) {
            std::cout << grid[y][x];
        }
        std::cout << "\n";
    }
}


void move_thread_func(std::atomic<bool>& stop_flag, std::vector<std::shared_ptr<NPC>>& npcs) {
    std::random_device rd;
    std::mt19937 gen(rd());

    while (!stop_flag) {
        {
            std::shared_lock<std::shared_mutex> lock(npcs_mutex);
            for (auto& npc : npcs) {
                if (!npc->alive.load()) continue;

                auto stats = npc_stats.at(npc->type);
                std::uniform_int_distribution<int> dx(-stats.move_distance, stats.move_distance);
                std::uniform_int_distribution<int> dy(-stats.move_distance, stats.move_distance);

                npc->x = std::max(0, std::min(MAP_SIZE - 1, npc->x + dx(gen)));
                npc->y = std::max(0, std::min(MAP_SIZE - 1, npc->y + dy(gen)));

                for (auto& other : npcs) {
                    if (other == npc || !other->alive.load()) continue;
                    if (npc->is_close(other, static_cast<size_t>(stats.kill_distance))) {
                        FightContext ctx;
                        ctx.attacker = npc;
                        ctx.defender = other;
                        ctx.kill_defender = false;
                        other->accept(std::make_shared<FightVisitorImpl>(ctx));

                        if (ctx.kill_defender) {
                            std::lock_guard<std::mutex> q_lock(fight_queue_mutex);
                            fight_queue.emplace(npc, other);
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}


void fight_thread_func(std::atomic<bool>& stop_flag) {
    while (!stop_flag) {
        std::queue<std::pair<std::shared_ptr<NPC>, std::shared_ptr<NPC>>> local_queue;
        {
            std::lock_guard<std::mutex> q_lock(fight_queue_mutex);
            if (fight_queue.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            local_queue = std::move(fight_queue);
        }

        while (!local_queue.empty()) {
            auto [attacker, defender] = local_queue.front();
            local_queue.pop();

            if (!attacker->alive.load() || !defender->alive.load()) continue;

            FightContext ctx;
            ctx.attacker = attacker;
            ctx.defender = defender;
            ctx.kill_defender = false;
            defender->accept(std::make_shared<FightVisitorImpl>(ctx));

            if (ctx.kill_defender) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<int> die(1, 6);
                int a = die(gen), d = die(gen);
                if (a > d) {
                    defender->alive.store(false);
                    attacker->fight_notify(defender, true);
                }
                
            }
        }
    }
}


int main() {
    auto npcs = generate_npcs();
    std::atomic<bool> stop_flag{false};

    std::thread move_thread(move_thread_func, std::ref(stop_flag), std::ref(npcs));
    std::thread fight_thread(fight_thread_func, std::ref(stop_flag));

    for (int sec = 1; sec <= GAME_DURATION_SEC; ++sec) {
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\n=== FRAME " << sec << " ===\n";
            print_map(npcs);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    stop_flag = true;
    move_thread.join();
    fight_thread.join();

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\n=== SURVIVORS ===\n";
        std::shared_lock<std::shared_mutex> lock2(npcs_mutex);
        for (const auto& n : npcs) {
            if (n->alive.load()) {
                n->print();
            }
        }
    }

    return 0;
}