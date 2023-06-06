#include "benchmark/benchmark.h"
#include "moderndbs/btree.h"
#include <barrier>
#include <random>
#include <thread>
#include <vector>

using BufferManager = moderndbs::BufferManager;
using BTree = moderndbs::BTree<uint64_t, uint64_t, std::less<>, 1024>;

void Btree_Multi(benchmark::State& state) {
   const int NUM_THREADS = 4;
   for (auto _ : state) {
      BufferManager buffer_manager(1024, 100);
      BTree tree(0, buffer_manager);

      std::barrier sync_point(NUM_THREADS);
      std::vector<std::thread> threads;
      for (size_t thread = 0; thread < NUM_THREADS; ++thread) {
         threads.emplace_back([thread, &sync_point, &tree] {
            size_t startValue = thread * 16 * BTree::LeafNode::kCapacity;
            size_t limit = startValue + 16 * BTree::LeafNode::kCapacity;
            // Insert values
            for (auto i = startValue; i < limit; ++i) {
               tree.insert(i, 2 * i);
            }
            sync_point.arrive_and_wait();
            std::mt19937_64 engine{thread};
            //90% inserts, 10% deletions
            std::bernoulli_distribution insert_distr{0.9};
            std::uniform_int_distribution<uint64_t> keys{0, NUM_THREADS * 16 * BTree::LeafNode::kCapacity};
            for (int i = 0; i < 1000; ++i) {
               if (insert_distr(engine)) {
                  auto key = keys(engine);
                  tree.insert(key, 0);
               } else {
                  tree.erase(keys(engine));
               }
            }
         });
      }
      for (auto& t : threads)
         t.join();
   }
}
BENCHMARK(Btree_Multi)->UseRealTime()->MinTime(10);
