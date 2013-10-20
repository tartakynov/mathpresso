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

//! @brief Operator associativity
enum OperatorAssoc
{
  NoAssoc,
  LeftAssoc,
  RightAssoc
};

//! @brief Struct for operator priority and associativity
struct OperatorInfo
{
  int priority;
  OperatorAssoc assoc;
};

//! @brief Operator info table
static const OperatorInfo mpOperatorInfo[] =
{
  { 0,  LeftAssoc  }, // MOPERATOR_NONE
  { 5,  RightAssoc }, // MOPERATOR_ASSIGN
  { 10, LeftAssoc  }, // MOPERATOR_PLUS
  { 10, LeftAssoc  }, // MOPERATOR_MINUS
  { 15, LeftAssoc  }, // MOPERATOR_MUL
  { 15, LeftAssoc  }, // MOPERATOR_DIV
  { 15, LeftAssoc  }, // MOPERATOR_MOD
  { 20, RightAssoc }, // MOPERATOR_POW
  { 25, RightAssoc }  // MOPERATOR_UMINUS
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
        // Skip semicolon and parse next expression
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
    block->getChildrenVector().swap(elements);

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
          // Operand expected
          result = MRESULT_EXPRESSION_EXPECTED;
          goto failure;
        }

        _tokenizer.back(&token);
        *dst = left;
        return MRESULT_OK;
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      case MTOKEN_INTEGER:
      case MTOKEN_FLOAT:
        if (left != NULL && op == MOPERATOR_NONE)
        {
          // Expected operator or end of expression
          result = MRESULT_UNEXPECTED_TOKEN;
          goto failure;
        }

        right = new ASTConstant(_ctx.genId(), token.f);
        break;
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      case MTOKEN_LPAREN:
        if (left != NULL && op == MOPERATOR_NONE)
        {
          // Expected operator or end of expression
          result = MRESULT_UNEXPECTED_TOKEN;
          goto failure;
        }

        result = parseExpression(&right, NULL, 0, true);
        if (result != MRESULT_OK)
          goto failure;

        _tokenizer.next(&token);
        if (token.tokenType != MTOKEN_RPAREN)
        {
          result = MRESULT_UNEXPECTED_TOKEN;
          goto failure;
        }

        break;
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      case MTOKEN_RPAREN:
        if (op != MOPERATOR_NONE)
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
          /* Why not?
          // Assignment inside expression is not allowed.
          if (isInsideExpression)
          {
            result = MRESULT_ASSIGNMENT_INSIDE_EXPRESSION;
            goto failure;
          }
          */

          // Can assign only to a variable
          if (left == NULL || left->getElementType() != MELEMENT_VARIABLE)
          {
            result = MRESULT_ASSIGNMENT_TO_NON_VARIABLE;
            goto failure;
          }
        }

        if (op != MOPERATOR_NONE || left == NULL)
        {
          if (token.operatorType == MOPERATOR_PLUS)
            // Ignore unary plus
            continue;

          if (token.operatorType == MOPERATOR_MINUS)
          {
            // Unary minus
            result = parseExpression(&right, right, mpOperatorInfo[MOPERATOR_UMINUS].priority, true);
            if (result != MRESULT_OK)
              goto failure;

            if (right == NULL)
            {
              result = MRESULT_EXPRESSION_EXPECTED;
              goto failure;
            }

            ASTTransform* transform = new ASTTransform(_ctx.genId());
            transform->setTransformType(MTRANSFORM_NEGATE);
            transform->setChild(right);
            right = transform;
            break;
		  }
          result = MRESULT_UNEXPECTED_TOKEN;
          goto failure;
		}

        op = token.operatorType;
		if (mpOperatorInfo[op].priority < minPriority || (mpOperatorInfo[op].priority == minPriority && mpOperatorInfo[op].assoc == LeftAssoc))
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
        if (left != NULL && op == MOPERATOR_NONE)
        {
          // Expected operator or end of expression
          result = MRESULT_UNEXPECTED_TOKEN;
          goto failure;
        }

        const char* symbolName = _tokenizer.beg + token.pos;
        size_t symbolLength = token.len;

        Token ttoken;
        bool isFunction = (_tokenizer.peek(&ttoken) == MTOKEN_LPAREN);

        // Parse function
        if (isFunction)
        {
          Function* function = _ctx._ctx->functions.get(symbolName, symbolLength);
          if (function == NULL)
          {
            result = MRESULT_INVALID_FUNCTION;
            goto failure;
          }

          // Parse LPAREN token again
          _tokenizer.next(&ttoken);

          Vector<ASTElement*> arguments;

          // Parse function arguments
          int numArgs = function->getArgumentsCount();
          int n = 0;
          for (;;++n)
          {
            _tokenizer.next(&ttoken);

            if (ttoken.tokenType == MTOKEN_ERROR)
            {
              mpDeleteAll(arguments);
              result = MRESULT_INVALID_TOKEN;
              goto failure;
            }

            // ')' - End of function call
            if (ttoken.tokenType == MTOKEN_RPAREN)
            {
              if (n == numArgs) break;

              mpDeleteAll(arguments);
              result = MRESULT_NOT_ENOUGH_ARGUMENTS;
              goto failure;
            }

            // ',' - Arguments delimiter
            if (n != 0)
            {
              if (ttoken.tokenType == MTOKEN_COMMA)
              {
                if (n >= numArgs)
                {
                  mpDeleteAll(arguments);
                  result = MRESULT_TOO_MANY_ARGUMENTS;
                  goto failure;
                }
              }
              else
              {
                mpDeleteAll(arguments);
                result = MRESULT_UNEXPECTED_TOKEN;
                goto failure;
              }
            }
            else
            {
              _tokenizer.back(&ttoken);
            }

            // Parse argument expression
            ASTElement* arg;
            if ((result = parseExpression(&arg, NULL, 0, true)) != MRESULT_OK)
            {
              mpDeleteAll(arguments);
              goto failure;
            }

            arguments.append(arg);
          }

          // Done
          ASTCall* call = new ASTCall(_ctx.genId(), function);
          call->swapArguments(arguments);
          right = call;
        }
        else
        // Parse variable
        {
          Variable* var = _ctx._ctx->variables.get(symbolName, symbolLength);
          if (var == NULL)
          {
            result = MRESULT_INVALID_SYMBOL;
            goto failure;
          }

          if (var->type == MVARIABLE_CONSTANT)
            right = new ASTConstant(_ctx.genId(), var->c.value);
          else
            right = new ASTVariable(_ctx.genId(), var);
        }

        break;
      }
      // ----------------------------------------------------------------------

      // ----------------------------------------------------------------------
      default:
        MP_ASSERT_NOT_REACHED();
      // ----------------------------------------------------------------------
    }

	MP_ASSERT(right != NULL);

    if (left)
    {
      _tokenizer.peek(&helper);

      if (helper.tokenType == MTOKEN_OPERATOR)
      {
        int lp = mpOperatorInfo[op].priority;
        int np = mpOperatorInfo[helper.operatorType].priority;
        if (lp < np || (lp == np && mpOperatorInfo[op].assoc == RightAssoc))
        {
          result = parseExpression(&right, right, lp, true);
          if (result != MRESULT_OK)
            goto failure;
        }
      }

      MP_ASSERT(op != MOPERATOR_NONE);
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
