#include <algorithm>
#include <array>
#include <cassert>
#include <deque>
#include <iostream>
#include <list>
#include <numeric>
#include <vector>

int main()
{
    std::array<int, 4> fixed_samples{101, 103, 102, 104};
    const int fixed_sum = std::accumulate(fixed_samples.begin(), fixed_samples.end(), 0);
    assert(fixed_sum == 410);

    std::vector<int> dynamic_samples;
    dynamic_samples.reserve(4);
    for (int sample : fixed_samples) {
        dynamic_samples.push_back(sample);
    }

    assert(dynamic_samples.size() == 4);
    assert(dynamic_samples.capacity() >= dynamic_samples.size());

    std::sort(dynamic_samples.begin(), dynamic_samples.end());
    assert(dynamic_samples.front() == 101);
    assert(dynamic_samples.back() == 104);

    std::deque<int> work_queue;
    work_queue.push_back(2);
    work_queue.push_front(1);
    work_queue.push_back(3);
    assert(work_queue.front() == 1);
    assert(work_queue.back() == 3);

    std::list<int> stable_nodes{1, 3, 4};
    auto insert_before = std::next(stable_nodes.begin());
    stable_nodes.insert(insert_before, 2);
    assert(std::is_sorted(stable_nodes.begin(), stable_nodes.end()));

    std::cout << "array-sum=" << fixed_sum
              << " vector-size=" << dynamic_samples.size()
              << " deque-front=" << work_queue.front()
              << " list-sorted=" << std::boolalpha
              << std::is_sorted(stable_nodes.begin(), stable_nodes.end())
              << " result=passed\n";
}
