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
#include "MathPresso_Util_p.h"

namespace MathPresso {

// ============================================================================
// [MathPresso::ASTElement]
// ============================================================================

ASTElement::ASTElement(uint elementId, uint elementType) :
  _parent(NULL),
  _elementId(elementId),
  _elementType(elementType)
{
}

ASTElement::~ASTElement()
{
}


bool ASTElement::replaceChild(ASTElement* child, ASTElement* element)
{
  ASTElement** children = getChildrenElements();
  size_t length = getChildrenCount();

  for (size_t i = 0; i < length; i++)
  {
    if (children[i] == child) { children[i] = element; return true; }
  }

  return false;
}

// ============================================================================
// [MathPresso::ASTBlock]
// ============================================================================

ASTBlock::ASTBlock(uint elementId) : ASTElement(elementId, MELEMENT_BLOCK)
{
}

ASTBlock::~ASTBlock()
{
  for (size_t i = 0; i < _elements.getLength(); i++) delete _elements[i];
}

bool ASTBlock::isConstant() const
{
  return false;
}

ASTElement** ASTBlock::getChildrenElements() const
{
  return const_cast<ASTElement**>(_elements.getData());
}

Vector<ASTElement *> & ASTBlock::getChildrenVector()
{
  return _elements;
}

size_t ASTBlock::getChildrenCount() const
{
  return _elements.getLength();
}

mreal_t ASTBlock::evaluate(void* data) const
{
  mreal_t result = 0;
  for (size_t i = 0; i < _elements.getLength(); i++)
  {
    result = _elements[i]->evaluate(data);
  }
  return result;
}

// ============================================================================
// [MathPresso::ASTNode]
// ============================================================================

ASTNode::ASTNode(uint elementId, uint elementType) :
  ASTElement(elementId, elementType),
  _left(NULL),
  _right(NULL)
{
}

ASTNode::~ASTNode()
{
  if (_left) delete _left;
  if (_right) delete _right;
}

bool ASTNode::isConstant() const
{
  return getLeft()->isConstant() && getRight()->isConstant();
}

ASTElement** ASTNode::getChildrenElements() const
{
  return const_cast<ASTElement**>(_elements);
}


size_t ASTNode::getChildrenCount() const
{
  return 2;
}

// ============================================================================
// [MathPresso::ASTConstant]
// ============================================================================

ASTConstant::ASTConstant(uint elementId, mreal_t value) :
  ASTElement(elementId, MELEMENT_CONSTANT),
  _value(value)
{
}

ASTConstant::~ASTConstant()
{
}

bool ASTConstant::isConstant() const
{
  return true;
}

ASTElement** ASTConstant::getChildrenElements() const
{
  return NULL;
}

size_t ASTConstant::getChildrenCount() const
{
  return 0;
}

bool ASTConstant::replaceChild(ASTElement* child, ASTElement* element)
{
  return false;
}

mreal_t ASTConstant::evaluate(void* data) const
{
  return _value;
}

// ============================================================================
// [MathPresso::ASTVariable]
// ============================================================================

ASTVariable::ASTVariable(uint elementId, const Variable* variable) :
  ASTElement(elementId, MELEMENT_VARIABLE),
  _variable(variable)
{
}

ASTVariable::~ASTVariable()
{
}

bool ASTVariable::isConstant() const
{
  return false;
}

ASTElement** ASTVariable::getChildrenElements() const
{
  return NULL;
}

size_t ASTVariable::getChildrenCount() const
{
  return 0;
}

bool ASTVariable::replaceChild(ASTElement* child, ASTElement* element)
{
  return false;
}

mreal_t ASTVariable::evaluate(void* data) const
{
  return reinterpret_cast<mreal_t*>((char*)data + getOffset())[0];
}

// ============================================================================
// [MathPresso::ASTOperator]
// ============================================================================

ASTOperator::ASTOperator(uint elementId, uint operatorType) :
  ASTNode(elementId, MELEMENT_OPERATOR),
  _operatorType(operatorType)
{
}

ASTOperator::~ASTOperator()
{
}

mreal_t ASTOperator::evaluate(void* data) const
{
  mreal_t result;

  switch (getOperatorType())
  {
    case MOPERATOR_ASSIGN:
    {
      MP_ASSERT(_left->getElementType() == MELEMENT_VARIABLE);
      result = _right->evaluate(data);
      reinterpret_cast<mreal_t*>((char*)data +
        reinterpret_cast<ASTVariable*>(_left)->getOffset())[0] = result;
      break;
    }
    case MOPERATOR_PLUS:
      result = _left->evaluate(data) + _right->evaluate(data);
      break;
    case MOPERATOR_MINUS:
      result = _left->evaluate(data) - _right->evaluate(data);
      break;
    case MOPERATOR_MUL:
      result = _left->evaluate(data) * _right->evaluate(data);
      break;
    case MOPERATOR_DIV:
      result = _left->evaluate(data) / _right->evaluate(data);
      break;
    case MOPERATOR_MOD:
    {
      mreal_t vl = _left->evaluate(data);
      mreal_t vr = _right->evaluate(data);
      result = fmod(vl, vr);
      break;
    }
    case MOPERATOR_POW:
    {
      mreal_t vl = _left->evaluate(data);
      mreal_t vr = _right->evaluate(data);
      result = pow(vl, vr);
      break;
    }
  }

  return result;
}

// ============================================================================
// [MathPresso::ASTCall]
// ============================================================================

ASTCall::ASTCall(uint elementId, Function* function) :
  ASTElement(elementId, MELEMENT_CALL),
  _function(function)
{
}

ASTCall::~ASTCall()
{
  mpDeleteAll(_arguments);
}

bool ASTCall::isConstant() const
{
  size_t i, len = _arguments.getLength();

  for (i = 0; i < len; i++)
  {
    if (!_arguments[i]->isConstant())
      return false;
  }

  return (getFunction()->getPrototype() & MFUNC_EVAL) != 0;
}

ASTElement** ASTCall::getChildrenElements() const
{
  return const_cast<ASTElement**>(_arguments.getData());
}

size_t ASTCall::getChildrenCount() const
{
  return _arguments.getLength();
}

mreal_t ASTCall::evaluate(void* data) const
{
  mreal_t result = 0.0f;
  mreal_t t[10];
  size_t i, len = _arguments.getLength();

  for (i = 0; i < len; i++)
  {
    t[i] = _arguments[i]->evaluate(data);
  }

  void* fn = getFunction()->getPtr();
  MP_ASSERT(getFunction()->getArguments() == len);

  switch (len)
  {
    case 0: result = ((MFunc_Ret_F_ARG0)fn)(); break;
    case 1: result = ((MFunc_Ret_F_ARG1)fn)(t[0]); break;
    case 2: result = ((MFunc_Ret_F_ARG2)fn)(t[0], t[1]); break;
    case 3: result = ((MFunc_Ret_F_ARG3)fn)(t[0], t[1], t[2]); break;
    case 4: result = ((MFunc_Ret_F_ARG4)fn)(t[0], t[1], t[2], t[3]); break;
    case 5: result = ((MFunc_Ret_F_ARG5)fn)(t[0], t[1], t[2], t[3], t[4]); break;
    case 6: result = ((MFunc_Ret_F_ARG6)fn)(t[0], t[1], t[2], t[3], t[4], t[5]); break;
    case 7: result = ((MFunc_Ret_F_ARG7)fn)(t[0], t[1], t[2], t[3], t[4], t[5], t[6]); break;
    case 8: result = ((MFunc_Ret_F_ARG8)fn)(t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7]); break;
  }

  return result;
}

// ============================================================================
// [MathPresso::ASTTransform]
// ============================================================================

ASTTransform::ASTTransform(uint elementId) :
  ASTElement(elementId, MELEMENT_TRANSFORM),
  _child(NULL),
  _transformType(MTRANSFORM_NONE)
{
}

ASTTransform::~ASTTransform()
{
  if (_child) delete _child;
}

bool ASTTransform::isConstant() const
{
  return getChild()->isConstant();
}

ASTElement** ASTTransform::getChildrenElements() const
{
  return const_cast<ASTElement**>(&_child);
}

size_t ASTTransform::getChildrenCount() const
{
  return 1;
}

mreal_t ASTTransform::evaluate(void* data) const
{
  mreal_t value = getChild()->evaluate(data);

  switch (getTransformType())
  {
    case MTRANSFORM_NONE:
      return value;
    case MTRANSFORM_NEGATE:
      return -value;
    default:
      MP_ASSERT_NOT_REACHED();
      return 0.0f;
  }
}

} // MathPresso namespace
