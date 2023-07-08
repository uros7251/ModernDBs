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
void ExpressionCompiler::compile(Expression& expression, bool /*verbose*/) {
   /// TODO compile a function for the expression
   auto& ctx = *context.getContext();
   module = std::make_unique<llvm::Module>("meaningful_module_name", ctx);
   llvm::IRBuilder<> builder(ctx);
   // function has to return 64-bit integer in any case
   auto* type = llvm::Type::getInt64Ty(ctx);
   auto exprT = llvm::FunctionType::get(type, {llvm::Type::getInt64PtrTy(ctx)}, false);
   auto exprFn = llvm::cast<llvm::Function>(module->getOrInsertFunction("_anon_expr_", exprT).getCallee());
   // Then, we generate the function logic.
   // entry:
   llvm::BasicBlock *exprFnEntryBlock = llvm::BasicBlock::Create(ctx, "entry", exprFn);
   builder.SetInsertPoint(exprFnEntryBlock);
   std::vector<llvm::Value *> exprFnArgs;
   // we expect a single arg, namely an array of int64_t elements 
   for(llvm::Function::arg_iterator ai = exprFn->arg_begin(), ae = exprFn->arg_end(); ai != ae; ++ai) {
      exprFnArgs.push_back(&*ai);
   }
   // build will generate code and store the result to exprFnTmp (it may recursively call build on children expressions)
   auto* exprFnTmp = expression.build(builder, exprFnArgs.empty() ? nullptr : *exprFnArgs.data());
   // if expression's type is already int64_t, just return it
   if (expression.type == Expression::ValueType::INT64) {
      builder.CreateRet(exprFnTmp);
   }
   // otherwise generate bitcast instruction first, and only then return
   else {
      builder.CreateRet(builder.CreateBitCast(exprFnTmp, type));
   }
   // dump the module (useful for debugging)
   // module->print(llvm::errs(), nullptr);
   jit.addModule(std::move(module));
   fnPtr = reinterpret_cast<data64_t (*)(data64_t*)>(jit.getPointerToFunction("_anon_expr_"));
}

/// Compile an expression.
data64_t ExpressionCompiler::run(data64_t* args) {
   auto x = fnPtr(args);
   return x;
}
data64_t moderndbs::Constant::evaluate(const data64_t* args) {
   return value;
}
llvm::Value* moderndbs::Constant::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   return type == ValueType::INT64 ? 
      llvm::ConstantInt::get(llvm::Type::getInt64Ty(builder.getContext()), value, true) :
      llvm::ConstantFP::get(llvm::Type::getDoubleTy(builder.getContext()), std::bit_cast<double>(value));
}

data64_t moderndbs::Argument::evaluate(const data64_t* args) {
   return args[index];
}

llvm::Value* moderndbs::Argument::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   auto& ctx = builder.getContext();
   auto* data_type = type == ValueType::INT64 ? llvm::Type::getInt64Ty(ctx) : llvm::Type::getDoubleTy(ctx);
   llvm::Value* offset = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), index, false);
   auto* address = builder.CreateGEP(llvm::Type::getInt64Ty(ctx), args, llvm::makeArrayRef({offset}));
   return builder.CreateLoad(data_type, address);
}

data64_t moderndbs::Cast::evaluate(const data64_t* args) {
   if (type == childType) {
      return args[0];
   }
   else if (childType == ValueType::INT64) {
      auto arg_int = std::bit_cast<int64_t>(args[0]);
      double arg_double = static_cast<double>(arg_int);
      return std::bit_cast<data64_t>(arg_double);
   }
   auto arg_double = std::bit_cast<double>(args[0]);
   double arg_int = static_cast<int64_t>(arg_double);
   return std::bit_cast<data64_t>(arg_int);
}

llvm::Value* moderndbs::Cast::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   auto* child_val = child.build(builder, args);
   if (type == childType) return child_val;
   else if (childType == ValueType::INT64) {
      return builder.CreateSIToFP(child_val, llvm::Type::getDoubleTy(builder.getContext()), "");
   }
   return builder.CreateFPToSI(child_val, llvm::Type::getInt64Ty(builder.getContext()), "");
}

data64_t moderndbs::AddExpression::evaluate(const data64_t* args) {
   if (left.type == ValueType::INT64) {
      auto left_int = std::bit_cast<int64_t>(left.evaluate(args));
      auto right_int = std::bit_cast<int64_t>(right.evaluate(args));
      return std::bit_cast<data64_t>(left_int+right_int);
   }
   auto left_fp = std::bit_cast<double>(left.evaluate(args));
   auto right_fp = std::bit_cast<double>(right.evaluate(args));
   return std::bit_cast<data64_t>(left_fp+right_fp);
}

llvm::Value* moderndbs::AddExpression::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   auto* left_val = left.build(builder, args);
   auto* right_val = right.build(builder, args);
   return left.type == ValueType::INT64 ? 
      builder.CreateAdd(left_val, right_val, "") :
      builder.CreateFAdd(left_val, right_val, "");
}

data64_t moderndbs::SubExpression::evaluate(const data64_t* args) {
   if (left.type == ValueType::INT64) {
      auto left_int = std::bit_cast<int64_t>(left.evaluate(args));
      auto right_int = std::bit_cast<int64_t>(right.evaluate(args));
      return std::bit_cast<data64_t>(left_int-right_int);
   }
   auto left_fp = std::bit_cast<double>(left.evaluate(args));
   auto right_fp = std::bit_cast<double>(right.evaluate(args));
   return std::bit_cast<data64_t>(left_fp-right_fp);
}

llvm::Value* moderndbs::SubExpression::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   auto* left_val = left.build(builder, args);
   auto* right_val = right.build(builder, args);
   return left.type == ValueType::INT64 ? 
      builder.CreateSub(left_val, right_val, "") :
      builder.CreateFSub(left_val, right_val, "");
}

data64_t moderndbs::MulExpression::evaluate(const data64_t* args) {
   if (left.type == ValueType::INT64) {
      auto left_int = std::bit_cast<int64_t>(left.evaluate(args));
      auto right_int = std::bit_cast<int64_t>(right.evaluate(args));
      return std::bit_cast<data64_t>(left_int*right_int);
   }
   auto left_fp = std::bit_cast<double>(left.evaluate(args));
   auto right_fp = std::bit_cast<double>(right.evaluate(args));
   return std::bit_cast<data64_t>(left_fp*right_fp);
}

llvm::Value* moderndbs::MulExpression::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   auto* left_val = left.build(builder, args);
   auto* right_val = right.build(builder, args);
   return left.type == ValueType::INT64 ? 
      builder.CreateMul(left_val, right_val, "") :
      builder.CreateFMul(left_val, right_val, "");
}

data64_t moderndbs::DivExpression::evaluate(const data64_t* args) {
   if (left.type == ValueType::INT64) {
      auto left_int = std::bit_cast<int64_t>(left.evaluate(args));
      auto right_int = std::bit_cast<int64_t>(right.evaluate(args));
      return std::bit_cast<data64_t>(left_int/right_int);
   }
   auto left_fp = std::bit_cast<double>(left.evaluate(args));
   auto right_fp = std::bit_cast<double>(right.evaluate(args));
   return std::bit_cast<data64_t>(left_fp/right_fp);
}

llvm::Value* moderndbs::DivExpression::build(llvm::IRBuilder<>& builder, llvm::Value* args) {
   auto* left_val = left.build(builder, args);
   auto* right_val = right.build(builder, args);
   return left.type == ValueType::INT64 ? 
      builder.CreateSDiv(left_val, right_val, "") :
      builder.CreateFDiv(left_val, right_val, "");
}
