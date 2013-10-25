# MathPresso - Mathematical Expression Evaluator And Jit Compiler for C++ Language
This is fork of MathPresso. Main project is located at http://code.google.com/p/mathpresso/

MathPresso is C++ library that is able to parse and evaluate mathematical
expressions. Because evaluating expressions can be slow MathPresso contains
jit compiler that can compile expressions into machine code. Jit compiled
expressions are many times faster than built-in evaluator. Thanks to AsmJit 
library MathPresso is able to compile functions for 32-bit (x86) and 64-bit
(x64) processors.

MathPresso is also an example how is possible to do with AsmJit library.

### Dependencies
AsmJit - 1.0 or later

### Example
Example how to compile and evaluate expression:
```cpp
#include <cstdlib>
#include <iostream>
#include <MathPresso/MathPresso.h>

using namespace std;

int main(void)
{
	MathPresso::Context ctx;
	MathPresso::Expression e;

	// values of variables used in expression
	MathPresso::mreal_t variables[] = { 1, 2 };

	// initialize the context with math environment (add math functions like sin, cos ..)
	ctx.addEnvironment(MathPresso::MENVIRONMENT_MATH);

	// add variables (name, offset)
	ctx.addVariable("x", 0 * sizeof(MathPresso::mreal_t));
	ctx.addVariable("y", 1 * sizeof(MathPresso::mreal_t));

	// compile an expression in specific context
	if (e.create(ctx, "pow(sin(x), 2) + pow(cos(x), 2)", MathPresso::MOPTION_NONE) != MathPresso::MRESULT_OK)
	{
		// handle compilation errr
		cerr << "Error compiling expression" << endl;
		return EXIT_FAILURE;
	}

	// if success then evaluate an expression 
	cout << e.evaluate(variables) << endl;

	return EXIT_SUCCESS;
}


```
### JIT compilation
Currently MathPresso is able to generate SSE or SSE2 instructions. The code in **default** branch generates SSE instructions and operates with single precision floating point numbers (SP-FP). If you need to perform an operations with double precision floating point (DP-FP, 64 bit) then you need to check out **DoublePresso** branch.

### Embedded functions
MathPresso supports following embedded functions:

* min(x, y), max(x, y), avg(x, y)
* ceil(x), floor(x), round(x)
* abs(x)
* reciprocal(x)
* sqrt(x), pow(x, y)
* log(x), log10(x)
* sin(x), cos(x), tan(x), sinh(x), cosh(x), tanh(x), asin(x), acos(x), atan(x), atan2(x)

If it's not enough you can add your function to the context:
```cpp
// implement your function
static mreal_t sum(mreal_t x, mreal_t y) { return x + y; }
...

// add your function to the context
ctx.addFunction("sum", (void *) sum, MathPresso::MFUNC_F_ARG2);
...

// and use it on expression
if (e.create(ctx, "1 + sum(x, y)", MathPresso::MOPTION_NONE) != MathPresso::MRESULT_OK)
{
...
```