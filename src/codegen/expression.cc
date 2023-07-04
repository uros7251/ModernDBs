#include "moderndbs/codegen/expression.h"
#include "moderndbs/error.h"

using JIT = moderndbs::JIT;
using Expression = moderndbs::Expression;
using ExpressionCompiler = moderndbs::ExpressionCompiler;
using data64_t = moderndbs::data64_t;

/// Evaluate the expresion.
data64_t Expression::evaluate(const data64_t* /*args*/) {
   throw NotImplementedException();
}

/// Build the expression
llvm::Value* Expression::build(llvm::IRBuilder<>& /*builder*/, llvm::Value* /*args*/) {
   throw NotImplementedException();
}

/// Constructor.
ExpressionCompiler::ExpressionCompiler(llvm::orc::ThreadSafeContext& context)
   : context(context), module(std::make_unique<llvm::Module>("meaningful_module_name", *context.getContext())), jit(context), fnPtr(nullptr) {}

/// Compile an expression.
void ExpressionCompiler::compile(Expression& /*expression*/, bool /*verbose*/) {
   /// TODO compile a function for the expression
   throw NotImplementedException();
}

/// Compile an expression.
data64_t ExpressionCompiler::run(data64_t* args) {
   return fnPtr(args);
}
