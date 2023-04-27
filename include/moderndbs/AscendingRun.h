#ifndef ASCENDING_RUN_H
#define ASCENDING_RUN_H
#include "file.h"

typedef uint64_t integer;

namespace moderndbs {

class ReadRun {
private:
    size_t remaining_num_values, in_mem_num_values;
    size_t start;
    int index;
    File* inputFile;
    integer* cache;
    size_t cache_len;
    // private methods
    void load();
public:
    ReadRun(File& input, size_t offset, size_t num_values, size_t memory_available);
    integer current() { return cache[index]; }
    void next();
    bool noMore() { return remaining_num_values <= 0; }
    ~ReadRun();
};

class WriteRun {
private:
    size_t in_mem_values;
    size_t start;
    int index;
    File* outputFile;
    integer* cache;
    size_t cache_len;
public:
    WriteRun(File& output, size_t offset, size_t memory_available);
    void push_back(integer);
    void flush();
    ~WriteRun();
};

}
#endif