// ---------------------------------------------------------------------------------------------------
// MODERNDBS
// ---------------------------------------------------------------------------------------------------
#include "benchmark/benchmark.h"
#include "moderndbs/buffer_manager.h"
#include <random>
#include <thread>
#include <vector>
// ---------------------------------------------------------------------------------------------------

namespace {

void BufferManager_Multi(benchmark::State& state) {
   for (auto _ : state) {
      moderndbs::BufferManager buffer_manager{1024, 10};
      std::vector<std::thread> threads;
      for (size_t i = 0; i < 10; ++i) {
         threads.emplace_back([i, &buffer_manager] {
            std::mt19937_64 engine{i};
            // 70% reads, 30% writes
            std::bernoulli_distribution reads_distr{0.7};
            // write a random amount of bytes / read a random byte
            std::uniform_int_distribution<uint64_t> distr{1, 1024};
            // access one of 5 segements
            std::uniform_int_distribution<uint64_t> segment_distr{0, 4};
            // with a hundred pages each
            std::uniform_int_distribution<uint16_t> page_distr{0, 99};
            for (size_t j = 0; j < 500; ++j) {
               uint64_t page_id = (static_cast<uint64_t>(segment_distr(engine)) << 48) | page_distr(engine);
               bool write_access = !reads_distr(engine);
               while (true) {
                  try {
                     auto& page = buffer_manager.fix_page(page_id, write_access);
                     auto tmp = *(page.get_data() + distr(engine) - 1);
                     benchmark::DoNotOptimize(tmp);
                     buffer_manager.unfix_page(page, write_access);
                     break;
                  } catch (const moderndbs::buffer_full_error&) {}
               }
            }
         });
      }
      for (auto& thread : threads) {
         thread.join();
      }
   }
}
} // namespace

BENCHMARK(BufferManager_Multi)->UseRealTime()->MinTime(10);
