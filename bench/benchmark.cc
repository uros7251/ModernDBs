#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/DynamicLibrary.h>
#include "benchmark/benchmark.h"

int main(int argc, char** argv) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
}
