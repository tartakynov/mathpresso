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

#ifndef _MATHPRESSO_PARSER_P_H
#define _MATHPRESSO_PARSER_P_H

#include "MathPresso.h"
#include "MathPresso_AST_p.h"
#include "MathPresso_Tokenizer_p.h"
#include "MathPresso_Util_p.h"

namespace MathPresso {

// ============================================================================
// [MathPresso::ExpressionParser]
// ============================================================================

//! @internal
//!
//! @brief Expression parser.
struct MATHPRESSO_HIDDEN ExpressionParser
{
  //! @brief Create a new @ref ExpressionParser instance.
  ExpressionParser(WorkContext& ctx, const char* input, size_t length);

  //! @brief Destroy the @ref ExpressionParser instance.
  ~ExpressionParser();

  //! @brief Parse complete expression.
  mresult_t parse(ASTElement** dst);

  //! @brief Parse single expression tree.
  mresult_t parseTree(ASTElement** dst);

  //! @brief Parse single expression, terminating on right paren or semicolon.
  mresult_t parseExpression(ASTElement** dst,
    ASTElement* _left,
    int minPriority,
    bool isInsideExpression);

protected:
  WorkContext& _ctx;

  Tokenizer _tokenizer;
  Token _last;
};

} // MathPresso namespace

#endif // _MATHPRESSO_PARSER_P_H
