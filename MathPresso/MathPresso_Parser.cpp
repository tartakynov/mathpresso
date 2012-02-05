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
#include "MathPresso_Parser_p.h"
#include "MathPresso_Tokenizer_p.h"
#include "MathPresso_Util_p.h"

namespace MathPresso {

// ============================================================================
// [Constants]
// ============================================================================

//! @internal
//!
//! @brief Operator priority.
static const int mpOperatorPriority[] =
{
  0 , // MOPERATOR_NONE
  5 , // MOPERATOR_ASSIGN
  10, // MOPERATOR_PLUS
  10, // MOPERATOR_MINUS
  15, // MOPERATOR_MUL
  15, // MOPERATOR_DIV
  15  // MOPERATOR_MOD
};

ExpressionParser::ExpressionParser(WorkContext& ctx, const char* input, size_t length) :
  _ctx(ctx),
  _tokenizer(input, length)
{
}

ExpressionParser::~ExpressionParser()
{
}

mresult_t ExpressionParser::parse(ASTElement** dst)
{
  return parseTree(dst);
}

mresult_t ExpressionParser::parseTree(ASTElement** dst)
{
  Vector<ASTElement*> elements;
  mresult_t result = MRESULT_OK;

  for (;;)
  {
    ASTElement* ast = NULL;
    if ((result = parseExpression(&ast, NULL, 0, false)) != MRESULT_OK)
      goto failed;
    if (ast) elements.append(ast);

    MP_ASSERT(_last.tokenType != MTOKEN_ERROR);
    switch (_last.tokenType)
    {
      case MTOKEN_END_OF_INPUT:
        goto finished;
      case MTOKEN_COMMA:
      case MTOKEN_RPAREN:
        result = MRESULT_UNEXPECTED_TOKEN;
        goto failed;
      case MTOKEN_SEMICOLON:
        // Skip semicolon and parse another expression.
        _tokenizer.next(&_last);
        break;
    }
  }

finished:
  if (elements.getLength() == 0)
  {
    return MRESULT_NO_EXPRESSION;
  }
  else if (elements.getLength() == 1)
  {
    *dst = elements[0];
    return MRESULT_OK;
  }
  else
  {
    ASTBlock* block = new ASTBlock(_ctx.genId());
    block->_elements.swap(elements);

    *dst = block;
    return MRESULT_OK;
  }

failed:
  for (size_t i = 0; i < elements.getLength(); i++) delete elements[i];
  return result;
}

mresult_t ExpressionParser::parseExpression(ASTElement** dst,
  ASTElement* _left,
  int minPriority,
  bool isInsideExpression)
{
  ASTElement* left = _left;
  ASTElement* right = NULL;

  mresult_t result = MRESULT_OK;
  uint op = MOPERATOR_NONE;
  uint om = MOPERATOR_NONE;

  Token& token = _last;
  Token helper;

  for (;;)
  {
    _tokenizer.next(&token);

    switch (token.tokenType)
    {
      // ----------------------------------------------------------------------
      case MTOKEN_ERROR:
        _tokenizer.back(&token);
        result = MRESULT_INVALID_TOKEN;
        goto failure;
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      case MTOKEN_COMMA:
      case MTOKEN_SEMICOLON:
        if (left == NULL)
        {
          return (isInsideExpression)
            ? MRESULT_UNEXPECTED_TOKEN
            : MRESULT_OK;
        }
        // ... fall through ...
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      case MTOKEN_END_OF_INPUT:
        if (op != MOPERATOR_NONE)
        {
          // Expecting expression.
          result = MRESULT_EXPECTED_EXPRESSION;
          goto failure;
        }

        _tokenizer.back(&token);
        *dst = left;
        return MRESULT_OK;
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      case MTOKEN_INTEGER:
      case MTOKEN_FLOAT:
        right = new ASTConstant(_ctx.genId(),
          om == MOPERATOR_MINUS ? -token.f : token.f);
        om = MOPERATOR_NONE;
        break;
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      case MTOKEN_LPAREN:
        result = parseExpression(&right, NULL, 0, true);
        if (result != MRESULT_OK) goto failure;

        _tokenizer.next(&token);
        if (token.tokenType != MTOKEN_RPAREN)
        {
          result = MRESULT_UNEXPECTED_TOKEN;
          goto failure;
        }

        if (om == MOPERATOR_MINUS)
        {
          ASTTransform* transform = new ASTTransform(_ctx.genId());
          transform->setTransformType(MTRANSFORM_NEGATE);
          transform->setChild(right);
          right = transform;
        }

        om = MOPERATOR_NONE;
        break;
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      case MTOKEN_RPAREN:
        if (op != MOPERATOR_NONE ||
            om != MOPERATOR_NONE)
        {
          result = MRESULT_UNEXPECTED_TOKEN;
          goto failure;
        }

        if (left == NULL && isInsideExpression)
        {
          result = MRESULT_UNEXPECTED_TOKEN;
          goto failure;
        }

        _tokenizer.back(&token);
        *dst = left;
        return MRESULT_OK;
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      case MTOKEN_OPERATOR:
        if (token.operatorType == MOPERATOR_ASSIGN)
        {
          // Assignment inside expression is not allowed.
          if (isInsideExpression)
          {
            result = MRESULT_ASSIGNMENT_INSIDE_EXPRESSION;
            goto failure;
          }
          // Must be assigned to an variable.
          if (left == NULL || left->getElementType() != MELEMENT_VARIABLE)
          {
            result = MRESULT_ASSIGNMENT_TO_NON_VARIABLE;
            goto failure;
          }
        }

        if (op == MOPERATOR_NONE && left == NULL)
        {
          om = token.operatorType;
          if (om == MOPERATOR_PLUS || om == MOPERATOR_MINUS) continue;

          result = MRESULT_UNEXPECTED_TOKEN;
          goto failure;
        }

        op = token.operatorType;
        if (mpOperatorPriority[op] < minPriority)
        {
          _tokenizer.back(&token);

          *dst = left;
          return MRESULT_OK;
        }
        continue;
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      case MTOKEN_SYMBOL:
      {
        const char* symbolName = _tokenizer.beg + token.pos;
        size_t symbolLength = token.len;

        Token ttoken;
        bool isFunction = (_tokenizer.peek(&ttoken) == MTOKEN_LPAREN);

        // Parse function.
        if (isFunction)
        {
          Function* function = _ctx._ctx->functions.get(symbolName, symbolLength);
          if (function == NULL)
          {
            result = MRESULT_NO_SYMBOL;
            goto failure;
          }

          // Parse LPAREN token again.
          _tokenizer.next(&ttoken);

          Vector<ASTElement*> arguments;
          bool first = true;

          // Parse function arguments.
          for (;;)
          {
            _tokenizer.next(&ttoken);

            if (ttoken.tokenType == MTOKEN_ERROR)
            {
              mpDeleteAll(arguments);
              result = MRESULT_INVALID_TOKEN;
              goto failure;
            }

            // ')' - End of function call.
            if (ttoken.tokenType == MTOKEN_RPAREN)
            {
              break;
            }

            // ',' - Arguments delimiter for non-first argument.
            if (!first)
            {
              if (ttoken.tokenType != MTOKEN_COMMA)
              {
                mpDeleteAll(arguments);
                result = MRESULT_EXPECTED_EXPRESSION;
                goto failure;
              }
            }
            else
            {
              _tokenizer.back(&ttoken);
            }

            // Expression.
            ASTElement* arg;
            if ((result = parseExpression(&arg, NULL, 0, true)) != MRESULT_OK)
            {
              mpDeleteAll(arguments);
              goto failure;
            }

            arguments.append(arg);
            first = false;
          }

          // Validate function arguments.
          if (function->getArguments() != arguments.getLength())
          {
            mpDeleteAll(arguments);
            result = MRESULT_ARGUMENTS_MISMATCH;
            goto failure;
          }

          // Done.
          ASTCall* call = new ASTCall(_ctx.genId(), function);
          call->swapArguments(arguments);
          right = call;
        }
        // Parse variable.
        else
        {
          Variable* var = _ctx._ctx->variables.get(symbolName, symbolLength);
          if (var == NULL)
          {
            result = MRESULT_NO_SYMBOL;
            goto failure;
          }

          if (var->type == MVARIABLE_CONSTANT)
            right = new ASTConstant(_ctx.genId(), var->c.value);
          else
            right = new ASTVariable(_ctx.genId(), var);
        }

        if (om == MOPERATOR_MINUS)
        {
          ASTTransform* transform = new ASTTransform(_ctx.genId());
          transform->setTransformType(MTRANSFORM_NEGATE);
          transform->setChild(right);
          right = transform;
        }

        om = MOPERATOR_NONE;
        break;
      }
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      default:
        MP_ASSERT_NOT_REACHED();
      // ----------------------------------------------------------------------
    }

    if (left)
    {
      _tokenizer.peek(&helper);

      if (helper.tokenType == MTOKEN_OPERATOR && mpOperatorPriority[op] < mpOperatorPriority[helper.operatorType])
      {
        result = parseExpression(&right, right, mpOperatorPriority[helper.operatorType], true);
        if (result != MRESULT_OK) { right = NULL; goto failure; }
      }

      ASTOperator* parent = new ASTOperator(_ctx.genId(), op);
      parent->setLeft(left);
      parent->setRight(right);

      left = parent;
      right = NULL;
      op = MOPERATOR_NONE;
    }
    else
    {
      left = right;
      right = NULL;
    }
  }

failure:
  if (left) delete left;
  if (right) delete right;

  *dst = NULL;
  return result;
}

} // MathPresso namespace
