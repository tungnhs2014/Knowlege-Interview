#include <cassert>
#include <cstddef>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct DeviceId {
    int bus{};
    int address{};

    bool operator==(const DeviceId& other) const
    {
        return bus == other.bus && address == other.address;
    }
};

struct DeviceIdHash {
    std::size_t operator()(const DeviceId& id) const
    {
        const std::size_t h1 = std::hash<int>{}(id.bus);
        const std::size_t h2 = std::hash<int>{}(id.address);
        return h1 ^ (h2 << 1);
    }
};

struct Task {
    std::string name;
    int priority{};
};

struct LowerPriorityFirst {
    bool operator()(const Task& lhs, const Task& rhs) const
    {
        return lhs.priority < rhs.priority;
    }
};

int main()
{
    const std::vector<std::string> events{"ok", "error", "ok", "warn", "error"};

    std::map<std::string, int> ordered_counts;
    std::unordered_map<std::string, int> fast_counts;
    fast_counts.reserve(events.size());

    for (const auto& event : events) {
        ++ordered_counts[event];
        ++fast_counts[event];
    }

    assert(ordered_counts.at("error") == 2);
    assert(fast_counts.at("ok") == 2);
    assert(ordered_counts.begin()->first == "error");

    std::unordered_set<DeviceId, DeviceIdHash> devices;
    devices.insert({1, 42});
    assert(devices.count({1, 42}) == 1);
    assert(devices.count({2, 42}) == 0);

    std::priority_queue<Task, std::vector<Task>, LowerPriorityFirst> tasks;
    tasks.push({"normal", 1});
    tasks.push({"urgent", 5});
    tasks.push({"background", 0});
    assert(tasks.top().name == "urgent");

    std::cout << "ordered-first=" << ordered_counts.begin()->first
              << " error-count=" << fast_counts.at("error")
              << " top-task=" << tasks.top().name
              << " result=passed\n";
}
