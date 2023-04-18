// ---------------------------------------------------------------------------------------------------
// MODERNDBS
// ---------------------------------------------------------------------------------------------------
#include "benchmark/benchmark.h"
#include "moderndbs/external_sort.h"
#include "moderndbs/file.h"
#include <cstring>
#include <random>
#include <vector>
// ---------------------------------------------------------------------------------------------------

namespace {

constexpr size_t NUM_VALUES = 20000;
constexpr size_t MEM_SIZE = 8 * 1 << 10;
 
void ExternalSort_Random(benchmark::State& state) {
   std::mt19937_64 engine{0};
   std::uniform_int_distribution<uint64_t> distr;
   std::vector<uint64_t> values;
   values.reserve(NUM_VALUES);
   for (size_t i = 0; i < NUM_VALUES; ++i) {
      uint64_t value = distr(engine);
      values.push_back(value);
   }
   auto file_content = std::make_unique<char[]>(NUM_VALUES * 8);
   std::memcpy(file_content.get(), values.data(), NUM_VALUES * 8);
   {
      auto write = moderndbs::PosixFile::open_file("input", moderndbs::File::WRITE);
      write->write_block(file_content.get(), 0, NUM_VALUES * 8);
   }
   auto input = moderndbs::PosixFile::open_file("input", moderndbs::File::READ);
   auto output = moderndbs::PosixFile::open_file("output", moderndbs::File::WRITE);
   for (auto _ : state) {
      moderndbs::external_sort(*input, NUM_VALUES, *output, MEM_SIZE);
   }
}
} // namespace

BENCHMARK(ExternalSort_Random)->UseRealTime()->MinTime(10);
