# ---------------------------------------------------------------------------
# MODERNDBS
# ---------------------------------------------------------------------------

set(BENCH_CC bench/bm_btree.cc)

add_executable(benchmarks bench/benchmark.cc ${BENCH_CC})
target_link_libraries(benchmarks moderndbs benchmark gtest gmock Threads::Threads)
