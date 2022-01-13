#pragma once

#include <new>
#include <memory>
#include "concurrent_trie.hpp"

template <typename T>
class concurrent_allocator {
    std::shared_ptr<concurrent_trie<>> trie;
    std::shared_ptr<T[]> memory;

public:
    concurrent_allocator(std::size_t size)
        : trie(std::make_shared<concurrent_trie<>>(size)),
        memory(new T[size]) {}

    struct thread_local_allocator {
        std::shared_ptr<concurrent_trie<>> trie;
        std::shared_ptr<T[]> memory;
        typename concurrent_trie<>::size_t size;
        typename concurrent_trie<>::size_t bits;
        decltype(trie->pop(size)) popper;

        thread_local_allocator(std::shared_ptr<concurrent_trie<>> trie, std::shared_ptr<T[]> memory)
            : trie(trie), memory(memory), bits(1) {}

        [[nodiscard]] T * allocate() {
            if (bits & (bits - 1))
                return memory.get() + popper(bits);
            popper = trie->pop(size);
            if (size == 0)
                // throw std::bad_alloc{};
                return nullptr;
            return memory.get() + popper(bits);
        }

        void deallocate(T *p) noexcept {
            auto i = p - memory.get();
            trie->push(i);
        }
    };
    
    thread_local_allocator get_thread_local_allocator() const {
        return thread_local_allocator(trie, memory);
    }

};