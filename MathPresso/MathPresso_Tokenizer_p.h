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

#ifndef _MATHPRESSO_TOKENIZER_P_H
#define _MATHPRESSO_TOKENIZER_P_H

#include "MathPresso.h"
#include "MathPresso_Util_p.h"

namespace MathPresso {

// ============================================================================
// [Constants]
// ============================================================================

//! @internal
//!
//! @brief Token type.
enum MTOKEN_TYPE
{
  MTOKEN_ERROR,
  MTOKEN_END_OF_INPUT,
  MTOKEN_INTEGER,
  MTOKEN_FLOAT,
  MTOKEN_COMMA,
  MTOKEN_LPAREN,
  MTOKEN_RPAREN,
  MTOKEN_OPERATOR,
  MTOKEN_SEMICOLON,
  MTOKEN_SYMBOL
};

// ============================================================================
// [MathPresso::Token]
// ============================================================================

struct Token
{
  // parser position from beginning of buffer and token length
  size_t pos;
  size_t len;

  // token type
  uint tokenType;

  // Token value.
  union
  {
    int operatorType;
    mreal_t f;
  };
};

// ============================================================================
// [MathPresso::Tokenizer]
// ============================================================================

struct Tokenizer
{
  Tokenizer(const char* input, size_t length);
  ~Tokenizer();

  uint next(Token* dst);
  uint peek(Token* dst);
  void back(Token* to);

  const char* beg;
  const char* cur;
  const char* end;
};

} // MathPresso namespace

#endif // _MATHPRESSO_TOKENIZER_P_H
