#pragma once

#include <atomic>
#include <bit>
#include <cstdint>
#include <climits>
#include <vector>
#include <queue>
#include <mutex>
#include <cassert>

template <class T = std::uint32_t, bool chunked = true>
class concurrent_trie {
public:
	using size_t = T;
	static constexpr T chunk_size = sizeof(T) * CHAR_BIT;
	static constexpr T chunk_mask = chunk_size - 1;
	static constexpr T chunk_bits = std::countr_zero(chunk_size);
	static constexpr T allset = ~(T)0;
	static constexpr T one = 1;
	T N = 0;
private:
	struct chunk {
		T bits;
		T version;
		chunk same_copy() const {
			return {bits, version + 1};
		}
		chunk cleared_copy(int offset) const {
			return {bits & ~(one << offset), version + 1};
		}
		chunk set_copy(int offset) const {
			return {bits | (one << offset), version + 1};
		}
	};
	int maxDepth = -1;
	T num_internal_nodes = 0;
	static_assert(std::atomic<chunk>::is_always_lock_free);
	std::vector<chunk> chunks;
	static_assert(std::atomic<T>::is_always_lock_free);
	std::vector<T> sizes;
	
	class popper {
		concurrent_trie *trie;
		T s;
	public:
		popper(concurrent_trie *trie = nullptr, T s = 0) : trie(trie), s(s) {}

		T operator ()(T &bits) { // if t has only 1 bit set, then next time will be empty
			int offset;
			{
				chunk n = std::atomic_ref(trie->chunks[s]).load();
				do {
					offset = std::countr_zero(n.bits);
					assert(offset >= 0); // after the changes, this should hold!
				} while(!std::atomic_ref(trie->chunks[s]).compare_exchange_weak(n, n.cleared_copy(offset)));
				bits = n.bits;
			}
			auto i = s - trie->num_internal_nodes;
			if constexpr(!chunked)
				bits = 1;
			return (i << chunk_bits) + offset;
		};
	};
public:
	concurrent_trie() {}
	inline T getChild(T i, T c) {
		return (i << chunk_bits) + c + 1;
	}
	inline T getParent(T i) {
		return (i - 1) >> chunk_bits;
	}
	concurrent_trie(T N) : N(N), maxDepth(N > 1 ? (chunk_size - std::countl_zero(N - 1) - 1) / chunk_bits : -1 + N), num_internal_nodes(maxDepth >= 0 ? ((one << (maxDepth * chunk_bits)) - 1) / (chunk_size - 1) : 0), chunks(num_internal_nodes + (N + chunk_size - 1) / chunk_size), sizes(chunks.size()) {
		auto level_start = num_internal_nodes;
		auto level_granularity = chunk_size;
		auto count_granularity = level_granularity >> chunk_bits;
		for(int d = maxDepth; d >= 0; d--) {
			const auto i_end = (N + (level_granularity - 1)) / level_granularity - 1;
			#pragma omp parallel for
			for(T i = 0; i < i_end; i++) {
				auto s = level_start + i;
				chunks[s] = chunk{allset, 0};
				if constexpr(chunked)
					sizes[s] = count_granularity;
				else
					sizes[s] = level_granularity;
			}
			auto i = (N - 1) / level_granularity;
			auto s = level_start + i;
			auto N_ = N - i * level_granularity;
			auto sz = (N_ + count_granularity - 1) / count_granularity;
			chunks[s] = chunk{allset >> (chunk_size - sz), 0};
			if constexpr(chunked)
				sizes[s] = (N_ + chunk_size - 1) >> chunk_bits;
			else
				sizes[s] = N_;
			count_granularity = level_granularity;
			level_granularity <<= chunk_bits;
			level_start = getParent(level_start);
		}
	}
	void push(T i) {//first | then increment size
		auto offset = i & chunk_mask;
		auto s = num_internal_nodes + (i >> chunk_bits);
		chunk n = std::atomic_ref(chunks[s]).load();
		while(!std::atomic_ref(chunks[s]).compare_exchange_weak(n, n.set_copy(offset)));
		bool flag;
		if constexpr(chunked)
			flag = n.bits == 0;
		else
			flag = (n.bits & (one << offset)) == 0;
		if(flag) {
			std::atomic_ref(sizes[s])++;
			for(int d = maxDepth - 1; d >= 0; d--) {
				s = getParent(s);
				i >>= chunk_bits;
				offset = i & chunk_mask;
				n = std::atomic_ref(chunks[s]).load();
				while(!std::atomic_ref(chunks[s]).compare_exchange_weak(n, n.set_copy(offset)));
				std::atomic_ref(sizes[s])++;
			}
		}
	}
	auto pop(T &sz) { // first decrement size at root, first decrement lower level size then & on current level
		T s = 0;
		if(maxDepth >= 0) {
			sz = std::atomic_ref(sizes[0]).load();
			do {
				if(sz == 0)
					break;
			} while(!std::atomic_ref(sizes[0]).compare_exchange_weak(sz, sz - 1));
			if(sz > 0) { // guaranteed to take one chunk by induction hypothesis
				T child;
				for(int d = 0; d < maxDepth; d++) {
					int offset;
					T child_sz;
					auto n = std::atomic_ref(chunks[s]).load();
					for(;;) {
						offset = std::countr_zero(n.bits);
						assert(offset >= 0);
						child = getChild(s, offset);
						child_sz = std::atomic_ref(sizes[child]).load();
						do {
							if(child_sz == 0)
								break;
						} while(!std::atomic_ref(sizes[child]).compare_exchange_weak(child_sz, child_sz - 1));
						if(child_sz >= 1)	// expect this to hold sooner or later because of inductive hypothesis
							break;
						std::atomic_ref(chunks[s]).compare_exchange_weak(n, n.cleared_copy(offset));
					}
					s = child;
				}
			}
		}
		else
			sz = 0;
		return popper(this, s);
	}
};
