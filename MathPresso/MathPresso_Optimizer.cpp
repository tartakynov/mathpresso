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
#include "MathPresso_Optimizer_p.h"
#include "MathPresso_Util_p.h"

namespace MathPresso {

Optimizer::Optimizer(WorkContext& ctx) :
  _ctx(ctx)
{
}

Optimizer::~Optimizer()
{
}

ASTElement* Optimizer::doNode(ASTElement* element)
{
  switch (element->getElementType())
  {
    case MELEMENT_BLOCK:
      return doBlock(reinterpret_cast<ASTBlock*>(element));
    case MELEMENT_OPERATOR:
      return doOperator(reinterpret_cast<ASTOperator*>(element));
    default:
      return element;
  }
}

ASTElement* Optimizer::doBlock(ASTBlock* element)
{
  return element;
}

ASTElement* Optimizer::doOperator(ASTOperator* element)
{
  ASTElement* left;
  ASTElement* right;

  left = element->_left = doNode(element->getLeft());
  right = element->_right = doNode(element->getRight());

  bool leftConst = left->isConstant();
  bool rightConst = right->isConstant();

  if (leftConst && rightConst)
  {
    // Both are constants, simplify them.
    mreal_t result = element->evaluate(NULL);

    ASTConstant* replacement = new ASTConstant(_ctx.genId(), result);
    replacement->_parent = element->getParent();
    delete element;
    return replacement;
  }
  else if (leftConst || rightConst)
  {
    // Left or right is constant, we try to find another one in deeper which
    // could be joined.
    ASTElement* c;
    ASTElement* x;

    if (leftConst)
    {
      c = left;
      x = right;
    }
    else
    {
      c = right;
      x = left;
    }

    ASTElement* y = findConstNode(x, element->getOperatorType());
    if (y != NULL)
    {
      ASTOperator* p = reinterpret_cast<ASTOperator*>(y->getParent());
      ASTElement* keep;

      MP_ASSERT(p->getElementType() == MELEMENT_OPERATOR);

      mreal_t result;

      // Hardcoded evaluator (we accept only PLUS or MUL operators).
      switch (element->getOperatorType())
      {
        case MOPERATOR_PLUS:
          result = c->evaluate(NULL) + y->evaluate(NULL);
          break;
        case MOPERATOR_MUL:
          result = c->evaluate(NULL) * y->evaluate(NULL);
          break;
        default:
          MP_ASSERT_NOT_REACHED();
      }

      if (p->getRight() == y)
      {
        keep = p->getLeft();
        p->_left = NULL;
      }
      else
      {
        keep = p->getRight();
        p->_right = NULL;
      }
      p->getParent()->replaceChild(p, keep);
      delete p;

      reinterpret_cast<ASTConstant*>(c)->setValue(result);
    }
  }
  return element;
}

ASTElement* Optimizer::findConstNode(ASTElement* _element, int op)
{
  if (_element->getElementType() != MELEMENT_OPERATOR) return NULL;
  ASTOperator* element = reinterpret_cast<ASTOperator*>(_element);

  int op0 = op;
  int op1 = element->getOperatorType();

  if ((op0 == MOPERATOR_PLUS && op1 == MOPERATOR_PLUS) ||
      (op0 == MOPERATOR_MUL  && op1 == MOPERATOR_MUL ) )
  {
    ASTElement* left = element->getLeft();
    ASTElement* right = element->getRight();

    if (left->isConstant()) return left;
    if (right->isConstant()) return right;

    if ((left = findConstNode(left, op)) != NULL) return left;
    if ((right = findConstNode(right, op)) != NULL) return right;
  }

  return NULL;
}

} // MathPresso namespace
