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
    if (argc >= 3)
        thread_count = std::stoull(argv[2]);
    std::atomic<std::size_t> total_count{};
    std::vector<std::size_t> counts(thread_count * 8, 0);
    for (int i = 0; i < thread_count; i++)
        thrs.emplace_back([&, id = i * 8]() {
        auto local_allocator = allocator.get_thread_local_allocator();
        std::vector<int *> v;
        for (std::size_t i = 0; i < size; i++) {
            v.push_back(local_allocator.allocate());
            // v.push_back(new int{});
            if (!v.back())
                break;
            local_allocator.deallocate(v.back());
            // delete v.back();
            v.push_back(local_allocator.allocate());
            // v.push_back(new int{});
            if (!v.back())
                break;
            total_count.fetch_add(1, std::memory_order_relaxed);
            counts[id]++;
        }
    });

    for (auto &t: thrs)
        t.join();

    std::cout << total_count.load() << std::endl;
    std::cout << std::accumulate(counts.begin(), counts.end(), std::size_t{}) << std::endl;
    // for (auto c: counts)
    //     std::cout << c << std::endl;

    return 0;
}