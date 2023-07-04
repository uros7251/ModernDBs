# Expressions

Implement an expression interpreter and an expression compiler for your database.
Your implementation should support the following type of expressions:

* **Constants**  
  Constants are either 8 byte float values or 8 byte signed integers.
* **Arguments**  
  Refers to a fixed index of an argument array storing 8 byte values (either floats or signed integers).
* **Cast**  
  Cast an expression from one type to another.
* **Addition**  
  Add two expressions.
* **Subtraction**  
  Subtract two expressions.
* **Multiplication**  
  Multiply two expressions.
* **Division**  
  Divide two expressions.

Add your implementation to the files  
`include/moderndbs/codegen/expression.h` and  
`src/moderndbs/codegen/expression.cc`.

## Interpreter
For the expression interpreter, each expression type should implement the method 
```c++
virtual data64_t evaluate(const data64_t* args)
```
which evaluates the expression tree given an argument array.

## Compiler
For the expression compiler, each expression type should implement the method 
```c++
virtual llvm::Value* build(llvm::IRBuilder<>& builder, llvm::Value* args)
```
which generates code for the expression using the LLVM framework.
The expression compiler should then implement the method `void compile(Expression& expression)` that compiles a function `data64_t (*fnPtr)(data64_t* args)` for the expression tree.
Use the class `include/moderndbs/codegen/jit.h` to simplify the LLVM module compilation significantly.
Refer to the simple [printf example](src/codegen/example/printf.cc) for information on how to generate a LLVM module.
For a long-form introduction to LLVM, see the [LLVM Frontend Tutorial](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html).

You don't need to implement implicit type casts.  
Subtrees of a binary expression (`Add`, `Sub`, `Mul`, `Div`) are guaranteed to have the same type: either a float or an integer.

## Testing
Your implementation should pass all tests in `test/expression_test.cc`.

Run the benchmark `bench/bm_expression.cc` to compare the efficiency of both approaches.

## Setup
This task requires LLVM 16 which must be installed separately.  
If you have trouble installing LLVM, please ask on Mattermost!

Ubuntu 23.04 (also works under the Windows Subsystem for Linux):
```sh
# Install LLVM 16 via apt
sudo apt install llvm-16 llvm-16-dev llvm-16-tools

# cmake now should find the dependencies automatically
```

MacOS:
```sh
# Install LLVM 16 via homebrew
brew install llvm@16

# However, we need to configure the project explicitly:
cmake -GNinja -DLLVM_DIR=/usr/local/Cellar/llvm/16.0.6/lib/cmake/llvm/ $PROJECT_DIRECTORY

# ...or set LLVM_DIR manually in CLion (File -> Settings -> Build, Execution, Deploy -> CMake -> CMake options) 
```

You can also use the docker image that we use on GitLab CI:
```sh
docker pull gitlab.db.in.tum.de:5005/infra/gpdi/image
```
More information here: https://gitlab.db.in.tum.de/infra/gpdi
