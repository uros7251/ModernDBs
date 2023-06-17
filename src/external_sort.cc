#include "moderndbs/external_sort.h"
#include "moderndbs/file.h"
#include "moderndbs/AscendingRun.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <sys/mman.h>

#define integer uint64_t

namespace moderndbs {

void inline read_to_vector(File& input, std::vector<integer>& vector, size_t offset, size_t num_values) {
    input.read_block(offset, num_values*sizeof(integer), reinterpret_cast<char*>(&vector[0]));
}

void inline write_from_vector(File& output, std::vector<integer>& vector, size_t offset, size_t num_values) {
    output.write_block(reinterpret_cast<const char*>(&vector[0]), offset, num_values*sizeof(integer));
}

void make_ascending_run(File& input, size_t input_offset, File& output, size_t output_offset, size_t num_values) {
    std::vector<integer> numbers(num_values, 0ul);
    read_to_vector(input, numbers, input_offset, num_values);
    std::sort(numbers.begin(), numbers.end());
    //for (auto x:numbers) std::cout << x << " " << std::endl;
    write_from_vector(output, numbers, output_offset, num_values);
}

void merge_runs(File& input, File& output, size_t offset_1, size_t num_values_1, size_t offset_2, size_t num_values_2, size_t offset_out, size_t mem_size) {
    ReadRun first(input, offset_1, num_values_1, mem_size/3),
        second(input, offset_2, num_values_2, mem_size/3);
    WriteRun out(output, offset_out, mem_size-(mem_size/3)*2);

    while (!first.noMore() && !second.noMore()) {
        if (first.current() < second.current()) {
            out.push_back(first.current());
            first.next();
        }
        else {
            out.push_back(second.current());
            second.next();
        }
    }
    if (first.noMore()) {
        while (!second.noMore()) {
            out.push_back(second.current());
            second.next();
        }
    }
    else {
        while (!first.noMore()) {
            out.push_back(first.current());
            first.next();
        }
    }
    out.flush();
}

void external_sort(File& input, size_t offset, size_t num_values, File& output, size_t mem_size, File& tmp) {
    auto mem_num_values = mem_size / sizeof(integer);
    if (num_values <= mem_num_values) {
        make_ascending_run(input, offset, output, offset, num_values);
    }
    else if (num_values < mem_num_values * 2) {
        make_ascending_run(input, offset, tmp, offset, mem_num_values);
        auto offset_displacement = mem_num_values*sizeof(integer);
        make_ascending_run(input, offset + offset_displacement, tmp, offset + offset_displacement, num_values-mem_num_values);
        merge_runs(
            tmp,
            output,
            offset,
            mem_num_values,
            offset+offset_displacement,
            num_values-mem_num_values,
            offset,
            mem_size);
    }
    else {
        external_sort(input, offset, num_values/2, tmp, mem_size, output);
        auto offset_displacement = (num_values/2)*sizeof(integer);
        external_sort(input, offset+offset_displacement, (num_values+1)/2, tmp, mem_size, output);
        merge_runs(
            tmp,
            output,
            offset,
            num_values/2,
            offset+offset_displacement,
            (num_values+1)/2,
            offset,
            mem_size);
    }
}

bool external_sort_with_mmap(File& input, size_t num_values, File& output) {
    if (PosixFile* posix = dynamic_cast<PosixFile*>(&output)) {
        auto ptr = mmap(NULL, posix->size(), PROT_WRITE, MAP_SHARED, posix->get_fd(), 0);
        if (ptr == MAP_FAILED) return false;
        input.read_block(0, input.size(), reinterpret_cast<char*>(ptr));
        auto numbers = reinterpret_cast<integer*>(ptr);
        std::sort(numbers, numbers + num_values);
        munmap(ptr, posix->size());
        return true;
    }
    return false;
}

void external_sort(File& input, size_t num_values, File& output, size_t mem_size) {
    // TODO: add your implementation here

    // check if output is big enough; if not, resize it
    // use uint_64
    if (mem_size < sizeof(integer)) return;
    if (output.size() != input.size()) {
        output.resize(input.size());
    }

    // if (external_sort_with_mmap(input, num_values, output)) return;

    size_t file_size = num_values * sizeof(integer); // in bytes
    size_t offset = 0;

    auto tmp = File::make_temporary_file();
    tmp->resize(file_size);
    external_sort(input, offset, num_values, output, mem_size, *tmp);

    // for (; offset+run_size < file_size; offset += run_size) {
    //     make_ascending_run(input, offset, output, offset, run_num_values);
    // }
    // if (offset < file_size) {
    //     make_ascending_run(input, offset, output, offset, (file_size-offset)/sizeof(integer));
    // }
    // // here we have sorted runs in output file
    // auto tmp_file = PosixFile::make_temporary_file();
    // tmp_file->resize(input.size());
    // offset = 0;

    // size_t remaining_num_values = num_values;
    // bool out_to_temp = true;
    // while (run_num_values < num_values) {
    //     File &out = out_to_temp ? *tmp_file : output,
    //         &in = out_to_temp ? output : *tmp_file;
    //     while (remaining_num_values > 2*run_num_values) {
    //         merge_runs(
    //             in,
    //             out,
    //             offset,
    //             run_num_values,
    //             offset + run_num_values*sizeof(integer),
    //             run_num_values,
    //             offset,
    //             mem_size);
    //         remaining_num_values -= 2*run_num_values;
    //         offset += run_num_values*sizeof(integer);
    //     }
    //     if (remaining_num_values > run_num_values) {
    //         merge_runs(
    //             in,
    //             out,
    //             offset,
    //             run_num_values,
    //             offset + run_num_values*sizeof(integer),
    //             remaining_num_values - run_num_values,
    //             offset,
    //             mem_size);
    //     }
    //     else {
    //         auto array = new integer[remaining_num_values];
    //         in.read_block(
    //             offset,
    //             remaining_num_values*sizeof(integer),
    //             reinterpret_cast<char*>(array));
    //         out.write_block(
    //             reinterpret_cast<char*>(array),
    //             offset,
    //             remaining_num_values*sizeof(integer));
    //         delete[] array;
    //     }
    //     offset = 0;
    //     run_num_values *= 2;
    //     remaining_num_values = num_values;
    //     out_to_temp = !out_to_temp;
    // }
}
}  // namespace moderndbs
