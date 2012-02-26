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
#include "MathPresso_DOT_p.h"
#include "MathPresso_Util_p.h"

namespace MathPresso {

// ============================================================================
// [MathPresso::DotBuilder]
// ============================================================================

struct DotBuilder
{
  DotBuilder(WorkContext& ctx);
  ~DotBuilder();

  void doTree(ASTElement* tree);
  void doElement(ASTElement* element);
  void doBlock(ASTBlock* element);
  void doConstant(ASTConstant* element);
  void doVariable(ASTVariable* element);
  void doOperator(ASTOperator* element);
  void doCall(ASTCall* element);
  void doTransform(ASTTransform* element);

  WorkContext& _ctx;
  StringBuilder _sb;
};

DotBuilder::DotBuilder(WorkContext& ctx) :
  _ctx(ctx)
{
}

DotBuilder::~DotBuilder()
{
}

void DotBuilder::doTree(ASTElement* tree)
{
  _sb.appendString("digraph G {\n");
  _sb.appendString("  node [shape=record];\n");
  doElement(tree);
  _sb.appendString("}\n");
}

void DotBuilder::doElement(ASTElement* element)
{
  switch (element->getElementType())
  {
    case MELEMENT_BLOCK:
      doBlock(reinterpret_cast<ASTBlock*>(element));
      break;
    case MELEMENT_CONSTANT:
      doConstant(reinterpret_cast<ASTConstant*>(element));
      break;
    case MELEMENT_VARIABLE:
      doVariable(reinterpret_cast<ASTVariable*>(element));
      break;
    case MELEMENT_OPERATOR:
      doOperator(reinterpret_cast<ASTOperator*>(element));
      break;
    case MELEMENT_CALL:
      doCall(reinterpret_cast<ASTCall*>(element));
      break;
    case MELEMENT_TRANSFORM:
      doTransform(reinterpret_cast<ASTTransform*>(element));
      break;
    default:
      MP_ASSERT_NOT_REACHED();
  }
}

void DotBuilder::doBlock(ASTBlock* element)
{
  Vector<ASTElement*>& children = element->getChildrenVector();
  size_t i, len = children.getLength();

  _sb.appendFormat("  N_%u [label=\"", element->getElementId());
  for (i = 0; i < len; i++) _sb.appendFormat("<F%u> |", (uint)i);
  _sb.appendFormat(" \"];\n", element->getElementId());

  for (i = 0; i < len; i++)
  {
    _sb.appendFormat("  N_%u:F%u -> N_%u:F0\n",
      element->getElementId(), (uint)i, children[i]->getElementId());
  }

  for (i = 0; i < len; i++)
  {
    doElement(children[i]);
  }
}

void DotBuilder::doConstant(ASTConstant* element)
{
  _sb.appendFormat("  N_%u [label=\"<F0>%f\"];\n", element->getElementId(), element->getValue());
}

void DotBuilder::doVariable(ASTVariable* element)
{
  _sb.appendFormat("  N_%u [label=\"<F0>", element->getElementId())
     .appendEscaped(Hash<Variable>::dataToKey(element->getVariable()))
     .appendString("\"];\n");
}

void DotBuilder::doOperator(ASTOperator* element)
{
  uint operatorType = element->getOperatorType();

  ASTElement* left  = element->getLeft();
  ASTElement* right = element->getRight();

  const char* opString = NULL;

  switch (operatorType)
  {
    case MOPERATOR_ASSIGN: opString = "="; break;
    case MOPERATOR_PLUS  : opString = "+"; break;
    case MOPERATOR_MINUS : opString = "-"; break;
    case MOPERATOR_MUL   : opString = "*"; break;
    case MOPERATOR_DIV   : opString = "/"; break;
    case MOPERATOR_MOD   : opString = "%"; break;
    case MOPERATOR_POW   : opString = "^"; break;
    default:
      MP_ASSERT_NOT_REACHED();
  }

  _sb.appendFormat("  N_%u [label=\"<L>|<F0>%s|<R>\"];\n", element->getElementId(), opString);
  _sb.appendFormat("  N_%u:L -> N_%u:F0;\n", element->getElementId(), left->getElementId());
  _sb.appendFormat("  N_%u:R -> N_%u:F0;\n", element->getElementId(), right->getElementId());

  doElement(left);
  doElement(right);
}

void DotBuilder::doCall(ASTCall* element)
{
  const Vector<ASTElement*>& arguments = element->getArguments();
  size_t i, len = arguments.getLength();
  size_t celter = len / 2;

  _sb.appendFormat("  N_%u [label=\"", element->getElementId());
  _sb.appendFormat("<F0>%s", Hash<Function>::dataToKey(element->getFunction()));
  for (i = 0; i < len; i++)
  {
    _sb.appendFormat("<A%u>", (unsigned int)i);
  }
  _sb.appendString("\"];\n");

  for (i = 0; i < len; i++)
  {
    _sb.appendFormat("  N_%u:A%u -> N_%u:F0;\n",
      element->getElementId(),
      (unsigned int)i,
      arguments[i]->getElementId());
  }

  for (i = 0; i < len; i++)
  {
    doElement(arguments[i]);
  }
}

void DotBuilder::doTransform(ASTTransform* element)
{
  uint transformType = element->getTransformType();
  ASTElement* child = element->getChild();

  const char* opString = NULL;

  switch (transformType)
  {
    case MTRANSFORM_NONE  : opString = ""; break;
    case MTRANSFORM_NEGATE: opString = "-"; break;
    default:
      MP_ASSERT_NOT_REACHED();
  }

  _sb.appendFormat("  N_%u [label=\"<F0>%s\"];\n", element->getElementId(), opString);
  _sb.appendFormat("  N_%u -> N_%u:F0;\n", element->getElementId(), child->getElementId());
  doElement(child);
}

MATHPRESSO_HIDDEN char* mpCreateDot(WorkContext& ctx, ASTElement* tree)
{
  DotBuilder builder(ctx);
  builder.doTree(tree);
  return builder._sb.toString();
}

} // MathPresso namespace
