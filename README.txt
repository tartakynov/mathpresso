MathPresso - Mathematical Expression Evaluator And Jit Compiler for C++ Language
================================================================================

http://code.google.com/p/mathpresso/

MathPresso is C++ library that is able to parse and evaluate mathematical
expressions. Because evaluating expressions can be slow MathPresso contains
jit compiler that can compile expressions into machine code. Jit compiled
expressions are many times faster than built-in evaluator. Thanks to AsmJit 
library MathPresso is able to compile functions for 32-bit (x86) and 64-bit
(x64) processors.

MathPresso is also an example how is possible to do with AsmJit library.

JIT Compilation
===============

In this fork MathPresso is modified to generate SSE2 instructions and operate with double precision floating point numbers (DP-FP) to be suitable for use in scientific computing. Current version is unstable and not recommended for production use.

Dependencies
============

AsmJit - 1.0 or later

Contact
=======

Author: Petr Kobalicek <kobalicek.petr@gmail.com>
Maintainer: Artem Tartakynov <artem.tad@gmail.com>
