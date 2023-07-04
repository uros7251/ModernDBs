#include "moderndbs/codegen/jit.h"
#include "moderndbs/error.h"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>

using JIT = moderndbs::JIT;

namespace {

static void optimizeModule(llvm::Module& module) {
    // Create a function pass manager
    auto passManager = std::make_unique<llvm::legacy::FunctionPassManager>(&module);

    // Add passes
    passManager->add(llvm::createInstructionCombiningPass());
    passManager->add(llvm::createReassociatePass());
    passManager->add(llvm::createGVNPass());
    passManager->add(llvm::createCFGSimplificationPass());
    passManager->doInitialization();

    // Run the optimizations
    for (auto& fn: module) {
        passManager->run(fn);
    }
}

}  // namespace

JIT::JIT(llvm::orc::ThreadSafeContext& ctx)
    : target_machine(llvm::EngineBuilder().selectTarget()),
      data_layout(target_machine->createDataLayout()),
      execution_session(std::make_unique<llvm::orc::UnsupportedExecutorProcessControl>()),
      context(ctx),
      object_layer(execution_session, []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
      compile_layer(execution_session, object_layer, std::make_unique<llvm::orc::SimpleCompiler>(*target_machine)),
      optimize_layer(execution_session, compile_layer, [] (llvm::orc::ThreadSafeModule m, const llvm::orc::MaterializationResponsibility&) { optimizeModule(*m.getModuleUnlocked()); return m; }),
      mainDylib(cantFail(execution_session.createJITDylib("<main>"), "createJITDylib failed")) {
    // Lookup symbols in host process
    auto generator = llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
        data_layout.getGlobalPrefix(),
        [](auto&) { return true; }));
    mainDylib.addGenerator(move(generator));
}

JIT::~JIT() {
    if (auto error = execution_session.endSession()) {
        execution_session.reportError(std::move(error));
    }
}

llvm::Error JIT::addModule(std::unique_ptr<llvm::Module> module) {
    if (llvm::verifyModule(*module, &llvm::errs())) {
       module->print(llvm::errs(), nullptr, false, true);
    }
    return optimize_layer.add(mainDylib, llvm::orc::ThreadSafeModule{move(module), context});
}

void* JIT::getPointerToFunction(std::string_view name) {
    auto sym = execution_session.lookup(&mainDylib, name);
    return sym ? reinterpret_cast<void*>(static_cast<uintptr_t>(sym->getAddress())) : nullptr;
}
