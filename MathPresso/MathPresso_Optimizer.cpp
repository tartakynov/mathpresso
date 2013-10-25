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

#include <limits>

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
    case MELEMENT_TRANSFORM:
      return doTransform(reinterpret_cast<ASTTransform*>(element));
    case MELEMENT_CALL:
      return doCall(reinterpret_cast<ASTCall*>(element));
    default:
      return element;
  }
}

ASTElement* Optimizer::doBlock(ASTBlock* element)
{
  ASTElement** elements = element->getChildrenElements();
  for (size_t i = 0; i < element->getChildrenCount(); ++i)
  {
    elements[i] = doNode(elements[i]);
  }
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
    replacement->getParent() = element->getParent();
    delete element;
    return replacement;
  }
  else if (leftConst || rightConst)
  {
    // Left or right is constant, try to find another one deeper that can be joined
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

    mreal_t cvalue = c->evaluate(NULL);
    if (cvalue == 0) // Optimize var*0, var+0, etc.
    {
      switch (element->getOperatorType())
      {
        case MOPERATOR_PLUS:
        { // x+0 == x
          element->replaceChild(x, NULL);
          delete element;
          return x;
        }
        case MOPERATOR_MUL:
        { // x*0 == 0
          element->replaceChild(c, NULL);
          delete element;
          return c;
        }
        case MOPERATOR_MINUS:
        {
          if (x == left) // x-0 == x
          {
            element->replaceChild(x, NULL);
            delete element;
            return x;
          }
          else // 0-x == -x
          {
            element->replaceChild(x, NULL);
            ASTTransform* replacement = new ASTTransform(_ctx.genId());
            replacement->setTransformType(MTRANSFORM_NEGATE);
            replacement->setChild(x);
            return replacement;
          }
        }
        case MOPERATOR_DIV:
        case MOPERATOR_MOD:
        {
          if (x == right) // 0/x == 0
          {
            element->replaceChild(c, NULL);
            delete element;
            return c;
          }
          else // x/0 - divide by zero!
          {
            // Not sure what to do here...

            //ASTConstant* replacement = new ASTConstant(_ctx.genId(), std::numeric_limits<double>::quiet_NaN());
            //replacement->getParent() = element->getParent();
            //delete element;
            //return replacement;
            break;
          }
        }
      }
    }
    else if (cvalue == 1) // Optimize var*1, var^1, etc.
    {
      switch (element->getOperatorType())
      {
        case MOPERATOR_MUL:
        { // x*1 == x
          element->replaceChild(x, NULL);
          delete element;
          return x;
        }
        case MOPERATOR_DIV:
        {
          if (x == left) // x/1 == x
          {
            element->replaceChild(x, NULL);
            delete element;
            return x;
          }
          else break;
        }
        case MOPERATOR_POW:
        {
          if (x == left) // x^1 == x
          {
            element->replaceChild(x, NULL);
            delete element;
            return x;
          }
          else // 1^x == 1
          {
            ASTConstant* replacement = new ASTConstant(_ctx.genId(), 1.0);
            replacement->getParent() = element->getParent();
            delete element;
            return replacement;
          }
        }
      }
    }
    else if (cvalue == -1) // Optimize var*-1, var/-1
    {
      switch (element->getOperatorType())
      {
        case MOPERATOR_DIV:
          if (x == right) break; // skip -1/x
          // fall through
        case MOPERATOR_MUL:
        {
          // -1*x == x*-1 == x/-1 == -x
          element->replaceChild(x, NULL);
          ASTTransform* replacement = new ASTTransform(_ctx.genId());
          replacement->setTransformType(MTRANSFORM_NEGATE);
          replacement->setChild(x);
          delete element;
          return replacement;
        }
      }
    }

    ASTElement* y = findConstNode(x, element->getOperatorType());
    if (y != NULL)
    {
      ASTOperator* p = reinterpret_cast<ASTOperator*>(y->getParent());
      ASTElement* keep;

      MP_ASSERT(p->getElementType() == MELEMENT_OPERATOR);

      mreal_t result;

      // Hardcoded evaluator (we accept only PLUS or MUL operators)
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
      c->getParent()->replaceChild(c, new ASTConstant(c->getElementId(), result));
      delete c;
    }
  }
  return element;
}

ASTElement* Optimizer::doCall(ASTCall* element)
{
  Vector<ASTElement*>& arguments = element->getArguments();
  size_t i, len = arguments.getLength();
  bool allConst = true;

  for (i = 0; i < len; i++)
  {
    arguments[i] = doNode(arguments[i]);
    allConst &= arguments[i]->isConstant();
  }

  if (allConst)
  {
    mreal_t result = element->evaluate(NULL);

    ASTElement* replacement = new ASTConstant(_ctx.genId(), result);
    replacement->getParent() = element->getParent();
    delete element;
    return replacement;
  }
  return element;
}

ASTElement* Optimizer::doTransform(ASTTransform* element)
{
  if (element->isConstant())
  {
    mreal_t result = element->evaluate(NULL);

    ASTElement* replacement = new ASTConstant(_ctx.genId(), result);
    replacement->getParent() = element->getParent();
    delete element;
    return replacement;
  }
  ASTElement* child = doNode(element->getChild());
  element->replaceChild(element->getChild(), child);

  switch (element->getTransformType())
  {
    case MTRANSFORM_NEGATE:
      if (child->getElementType() == MELEMENT_TRANSFORM)
      {
        ASTTransform* childTransform = reinterpret_cast<ASTTransform*>(child);
        if (childTransform->getTransformType() == MTRANSFORM_NEGATE)
        {
          ASTElement* childChild = childTransform->getChild();
          childTransform->removeChild();
          delete element;
          return childChild;
        }
      }
      break;
    case MTRANSFORM_NONE:
      // Should not happen
    default:
      MP_ASSERT_NOT_REACHED();
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
