#include <memory>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>
#include "moderndbs/codegen/examples.h"

/// A small example creating a LLVM IR module calling printf
/// You can use this function to create a first module for the JIT
std::unique_ptr<llvm::Module> moderndbs::buildPrintfExample(llvm::LLVMContext& ctx) {
    auto mod = std::make_unique<llvm::Module>("printfExample", ctx);
    llvm::IRBuilder<> builder(ctx);

    // We first need a function declarations for printf
    // declare i32 @printf(i8*, ...)
    auto printfT = llvm::FunctionType::get(llvm::Type::getInt32Ty(ctx), {llvm::Type::getInt8Ty(ctx)}, true);
    auto printfFn = llvm::cast<llvm::Function>(mod->getOrInsertFunction("printf", printfT).getCallee());

    // And a function *definition* for our own
    // define i32 @foo(i32 %x, i32 %y) {
    auto fooT = llvm::FunctionType::get(llvm::Type::getInt32Ty(ctx), {llvm::Type::getInt32Ty(ctx), llvm::Type::getInt32Ty(ctx)}, false);
    auto fooFn = llvm::cast<llvm::Function>(mod->getOrInsertFunction("foo", fooT).getCallee());

    // Then, we generate the function logic.
    // entry:
    // %tmp = mul i32 %x, %y
    llvm::BasicBlock *fooFnEntryBlock = llvm::BasicBlock::Create(ctx, "entry", fooFn);
    builder.SetInsertPoint(fooFnEntryBlock);
    std::vector<llvm::Value *> fooFnArgs;
    for(llvm::Function::arg_iterator ai = fooFn->arg_begin(), ae = fooFn->arg_end(); ai != ae; ++ai) {
        fooFnArgs.push_back(&*ai);
    }
    if(fooFnArgs.size() < 2) {
        throw("LLVM: foo() does not have enough arguments");
    }
    llvm::Value *fooFnTmp = builder.CreateMul(fooFnArgs[0] /* %x */, fooFnArgs[1] /* %y */);

    // Then we call: printf("%d\n", fooFnTmp)
    // call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @f, i32 0, i32 0), i32 %tmp)
    llvm::Value *fooFnStrPtr = builder.CreateGlobalStringPtr("%d\n", "f");
    std::vector<llvm::Value*> fprintArgs;
    fprintArgs.push_back(fooFnStrPtr);
    fprintArgs.push_back(fooFnTmp);
    builder.CreateCall(printfFn, llvm::makeArrayRef(fprintArgs));

    // ret i32 %tmp
    builder.CreateRet(fooFnTmp);

    // Dump the module (useful for debugging)
    mod->print(llvm::errs(), nullptr);
    return mod;
}
