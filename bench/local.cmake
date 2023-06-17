# ---------------------------------------------------------------------------
# MODERNDBS
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Files
# ---------------------------------------------------------------------------

set(BENCH_CC
    bench/bm_external_sort.cc
)

add_executable(benchmarks bench/benchmark.cc ${BENCH_CC})
target_link_libraries(benchmarks moderndbs benchmark gtest gmock Threads::Threads)
