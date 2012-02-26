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

// [Dependencies]
#include "MathPresso.h"
#include "MathPresso_AST_p.h"
#include "MathPresso_Tokenizer_p.h"
#include "MathPresso_Util_p.h"

namespace MathPresso {

// ============================================================================
// [MathPresso::Tokenizer]
// ============================================================================

Tokenizer::Tokenizer(const char* input, size_t length)
{
  beg = input;
  cur = input;
  end = input + length;
}

Tokenizer::~Tokenizer()
{
}

uint Tokenizer::next(Token* dst)
{
  // Skip spaces.
  while (cur != end && mpIsSpace(*cur)) *cur++;

  // End of input.
  if (cur == end)
  {
    dst->pos = (size_t)(cur - beg);
    dst->len = 0;
    dst->tokenType = MTOKEN_END_OF_INPUT;
    return MTOKEN_END_OF_INPUT;
  }

  // Save first input character for error handling and postparsing.
  const char* first = cur;

  // Type of token is usually determined by first input character.
  uint uc = *cur;

  // Parse float.
  if (mpIsDigit(uc))
  {
    uint t = MTOKEN_INTEGER;

    // Parse digit part.
    while (++cur != end)
    {
      uc = *cur;
      if (!mpIsDigit(uc)) break;
    }

    // Parse dot.
    if (cur != end && uc == '.')
    {
      while (++cur != end)
      {
        uc = *cur;
        if (!mpIsDigit(uc)) break;
        t = MTOKEN_FLOAT;
      }
    }

    dst->pos = (size_t)(first - beg);
    dst->len = (size_t)(cur - first);

    if (mpIsAlpha(uc)) goto error;

    // Convert string to float.
    bool ok;
    mreal_t n = mpConvertToFloat(first, (size_t)(cur - first), &ok);
    if (!ok) goto error;

    dst->tokenType = t;
    dst->f = n;
    return t;
  }

  // Parse symbol.
  else if (mpIsAlpha(uc) || uc == '_')
  {
    while (++cur != end)
    {
      uc = *cur;
      if (!(mpIsAlnum(uc) || uc == '_')) break;
    }

    dst->pos = (size_t)(first - beg);
    dst->len = (size_t)(cur - first);

    dst->tokenType = MTOKEN_SYMBOL;
    return MTOKEN_SYMBOL;
  }

  // Parse operators, parenthesis, etc...
  else
  {
    cur++;

    dst->pos = (size_t)(first - beg);
    dst->len = (size_t)(cur - first);

    switch (uc)
    {
      case ',': dst->tokenType = MTOKEN_COMMA; break;
      case '(': dst->tokenType = MTOKEN_LPAREN; break;
      case ')': dst->tokenType = MTOKEN_RPAREN; break;
      case ';': dst->tokenType = MTOKEN_SEMICOLON; break;
      case '=': dst->tokenType = MTOKEN_OPERATOR; dst->operatorType = MOPERATOR_ASSIGN; break;
      case '+': dst->tokenType = MTOKEN_OPERATOR; dst->operatorType = MOPERATOR_PLUS; break;
      case '-': dst->tokenType = MTOKEN_OPERATOR; dst->operatorType = MOPERATOR_MINUS; break;
      case '*': dst->tokenType = MTOKEN_OPERATOR; dst->operatorType = MOPERATOR_MUL; break;
      case '/': dst->tokenType = MTOKEN_OPERATOR; dst->operatorType = MOPERATOR_DIV; break;
      case '%': dst->tokenType = MTOKEN_OPERATOR; dst->operatorType = MOPERATOR_MOD; break;
      case '^': dst->tokenType = MTOKEN_OPERATOR; dst->operatorType = MOPERATOR_POW; break;
      default : dst->tokenType = MTOKEN_ERROR; break;
    }

    return dst->tokenType;
  }

error:
  dst->tokenType = MTOKEN_ERROR;
  cur = first;
  return MTOKEN_ERROR;
}

uint Tokenizer::peek(Token* to)
{
  next(to);
  back(to);
  return to->tokenType;
}

void Tokenizer::back(Token* to)
{
  cur = beg + to->pos;
}

} // MathPresso namespace
