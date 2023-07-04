#ifndef INCLUDE_MODERNDBS_CODEGEN_EXPRESSION_H
#define INCLUDE_MODERNDBS_CODEGEN_EXPRESSION_H

#include <memory>
#include "moderndbs/codegen/jit.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

namespace moderndbs {
    /// A value (can either be a signed (!) 64 bit integer or a double).
    /// You can std::bit_cast / llvm BitCast relatively freely between them.
    using data64_t = uint64_t;
    static_assert(sizeof(uint64_t) == sizeof(double));

    struct Expression {

        /// A result value type of the expression. We simplify the implementation
        /// by only supporting 64-bit values, opaquely represented by the data64_t
        enum class ValueType {
            INT64,
            DOUBLE
        };

        /// The type of the expression.
        ValueType type;

        /// Constructor.
        explicit Expression(ValueType type): type(type) {}
        /// Destructor
        virtual ~Expression() = default;

        /// Get the expression type.
        ValueType getType() { return type; }
        /// Evaluate the expression by interpreting it.
        /// @args: all function arguments that can be referenced by an @Argument
        virtual data64_t evaluate(const data64_t* args);
        /// Build the expression LLVM IR code.
        /// @args: all function arguments that can be referenced by an @Argument
        virtual llvm::Value* build(llvm::IRBuilder<>& builder, llvm::Value* args);
    };

    /// A constant value.
    struct Constant: public Expression {
        /// The constant value.
        data64_t value;

        /// Constructor.
        Constant(long long value)
            : Expression(ValueType::INT64), value(*reinterpret_cast<data64_t*>(&value)) {}
        /// Constructor.
        Constant(double value)
            : Expression(ValueType::DOUBLE), value(*reinterpret_cast<data64_t*>(&value)) {}

        /// TODO(students) implement evaluate and build
    };

    /// An Argument. E.g., x in the function: fn(x) = x + 42
    struct Argument: public Expression {
        /// The argument index.
        /// For fn(x, y, z): x == Argument(0), y == Argument(1), z = Argument(2)
        uint64_t index;

        /// Constructor.
        Argument(uint64_t index, ValueType type)
            : Expression(type), index(index) {}

        /// TODO(students) implement evaluate and build
    };

    /// A converting Cast. Note that this needs proper converting casts that transform the binary representation.
    /// E.g., 42.0 === 0x4045000000000000, but 42 == 0x000000000000002A
    struct Cast: public Expression {
        /// The child expression to cast.
        Expression& child;
        /// The type of the child.
        ValueType childType;

        /// Constructor.
        Cast(Expression& child, ValueType type)
            : Expression(type), child(child) {
            childType = child.getType();
        }

        /// TODO(students) implement evaluate and build
    };

    /// An abstract binary expression with two subexpressions
    struct BinaryExpression: public Expression {
        /// The left child.
        Expression& left;
        /// The right child.
        Expression& right;

        /// Constructor.
        BinaryExpression(Expression& left, Expression& right)
            : Expression(ValueType::INT64), left(left), right(right) {
            assert(left.getType() == right.getType() && "the left and right type must equal");
            type = left.getType();
        }
    };

    /// An addition. E.g., x + 42
    struct AddExpression: public BinaryExpression {
        /// Constructor
        AddExpression(Expression& left, Expression& right)
            : BinaryExpression(left, right) {}

        /// TODO(students) implement evaluate and build
    };

    /// A Subtraction. E.g., x - 42
    struct SubExpression: public BinaryExpression {
        /// Constructor
        SubExpression(Expression& left, Expression& right)
            : BinaryExpression(left, right) {}

        /// TODO(students) implement evaluate and build
    };

    /// A Multiplication. E.g., x * 42
    struct MulExpression: public BinaryExpression {
        /// Constructor
        MulExpression(Expression& left, Expression& right)
            : BinaryExpression(left, right) {}

        /// TODO(students) implement evaluate and build
    };

    /// A Division. E.g., x / 42
    /// Note that we don't test the edge-cases, but handling them would be nice.
    struct DivExpression: public BinaryExpression {
        /// Constructor
        DivExpression(Expression& left, Expression& right)
            : BinaryExpression(left, right) {}

        /// TODO(students) implement evaluate and build
    };

    /// The expression compiler that generates LLVM IR code and JIT compiles it.
    struct ExpressionCompiler {
        /// The llvm context managing LLVM's global data like types or constants.
        llvm::orc::ThreadSafeContext& context;
        /// The llvm module that contains functions of LLVM IR Basic Blocks.
        std::unique_ptr<llvm::Module> module;
        /// The jit.
        JIT jit;
        /// The compiled function. See JIT::getPointerToFunction().
        data64_t (*fnPtr)(data64_t* args);

        /// Constructor.
        explicit ExpressionCompiler(llvm::orc::ThreadSafeContext& context);

        /// Compile the expression by recursively calling Expression::build().
        /// @verbose: print the LLVM IR for debugging
        void compile(Expression& expression, bool verbose = false);
        /// Run a previously compiled expression.
        data64_t run(data64_t* args);
    };

}  // namespace moderndbs

#endif
