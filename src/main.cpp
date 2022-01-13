#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <numeric>

#include "concurrent_allocator.hpp"

int main(int argc, char *argv[]) {
    std::size_t size = 1024;
    if (argc >= 2)
        size = std::stoull(argv[1]);
    concurrent_allocator<int> allocator(size);
    std::vector<std::thread> thrs;
    std::size_t thread_count = 4;
    std::atomic<std::size_t> total_count{};
    std::vector<std::size_t> counts(thread_count, 0);
    for (int i = 0; i < thread_count; i++)
        thrs.emplace_back([&, id = i]() {
        auto local_allocator = allocator.get_thread_local_allocator();
        std::vector<int *> v;
        for (std::size_t i = 0; i < size; i++) {
            v.push_back(local_allocator.allocate());
            if (!v.back())
                break;
            total_count++;
            counts[id]++;
            local_allocator.deallocate(v.back());
            total_count--;
            counts[id]--;
            v.push_back(local_allocator.allocate());
            if (!v.back())
                break;
            total_count++;
            counts[id]++;
        }
    });

    for (auto &t: thrs)
        t.join();

    std::cout << total_count.load() << std::endl;
    std::cout << std::accumulate(counts.begin(), counts.end(), std::size_t{}) << std::endl;
    for (auto c: counts)
        std::cout << c << std::endl;

    return 0;
}