// MathPresso - Mathematical Expression Parser and JIT Compiler.

// Copyright (c) 2008-2010, Petr Kobalicek <kobalicek.petr@gmail.com>
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#ifndef _MATHPRESSO_OPTIMIZER_P_H
#define _MATHPRESSO_OPTIMIZER_P_H

#include "MathPresso.h"
#include "MathPresso_AST_p.h"
#include "MathPresso_Util_p.h"

namespace MathPresso {

// ============================================================================
// [MathPresso::ExpressionSimplifier]
// ============================================================================

//! @internal
//!
//! @brief Simplifies expression tree by evaluating constant nodes
class Optimizer
{
  WorkContext& _ctx;

public:
  Optimizer(WorkContext& ctx);
  ~Optimizer();

  inline void optimize(ASTElement* &element) { element = doNode(element); }

protected:
  ASTElement* doNode(ASTElement* element);
  ASTElement* doBlock(ASTBlock* element);
  ASTElement* doOperator(ASTOperator* element);
  ASTElement* doCall(ASTCall* element);
  ASTElement* doTransform(ASTTransform* element);

  ASTElement* findConstNode(ASTElement* element, int op);
};

} // MathPresso namespace

#endif // _MATHPRESSO_OPTIMIZER_P_H
