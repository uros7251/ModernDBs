#include "moderndbs/AscendingRun.h"
#include <stdexcept>
#include <iostream>

moderndbs::ReadRun::ReadRun(File& input, size_t offset, size_t num_values, size_t memory_available)
    :inputFile(&input),
    start(offset),
    remaining_num_values(num_values),
    in_mem_num_values(memory_available/sizeof(integer)),
    index(0)
{
    if (in_mem_num_values == 0) {
        throw std::runtime_error("Memory too small!");
    }
    cache = new integer[in_mem_num_values];
    load();
}

void moderndbs::ReadRun::load() {
    cache_len = remaining_num_values > in_mem_num_values ? in_mem_num_values : remaining_num_values;
    inputFile->read_block(
        start,
        cache_len*sizeof(integer),
        reinterpret_cast<char*>(cache));
    start += cache_len*sizeof(integer);
}

void moderndbs::ReadRun::next() {
    --remaining_num_values;
    if (index+1 < cache_len) {
        ++index;
    }
    else if (remaining_num_values > 0) {
        load();
        index = 0;
    }
}

moderndbs::ReadRun::~ReadRun() {
    delete[] cache;
}

moderndbs::WriteRun::WriteRun(File& output, size_t offset, size_t memory_available)
    :outputFile(&output),
    start(offset),
    in_mem_values(memory_available/sizeof(integer)),
    cache_len(0)
{
    cache = new integer[in_mem_values];
}

void moderndbs::WriteRun::push_back(integer a) {
    if (cache_len == in_mem_values) {
        flush();
    }
    cache[cache_len++] = a;
}

void moderndbs::WriteRun::flush() {
    if (cache_len == 0) return;
    outputFile->write_block(
        reinterpret_cast<const char*>(cache),
        start,
        cache_len*sizeof(integer));
    start += cache_len*sizeof(integer);
    cache_len = 0;
}

moderndbs::WriteRun::~WriteRun() {
    delete[] cache;
}
