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

#ifndef _MATHPRESSO_AST_P_H
#define _MATHPRESSO_AST_P_H

#include "MathPresso.h"
#include "MathPresso_Context_p.h"
#include "MathPresso_Util_p.h"

namespace MathPresso {

// ============================================================================
// [MathPresso::Constants]
// ============================================================================

//! @internal
//!
//! @brief Element type.
enum MELEMENT_TYPE
{
  MELEMENT_BLOCK,
  MELEMENT_CONSTANT,
  MELEMENT_VARIABLE,
  MELEMENT_OPERATOR,
  MELEMENT_CALL,
  MELEMENT_TRANSFORM
};

//! @internal
//!
//! @brief Operator type.
enum MOPERATOR_TYPE
{
  MOPERATOR_NONE,
  MOPERATOR_ASSIGN,
  MOPERATOR_PLUS,
  MOPERATOR_MINUS,
  MOPERATOR_MUL,
  MOPERATOR_DIV,
  MOPERATOR_MOD
};

//! @internal
//!
//! @brief Transform type.
enum MTRANSFORM_TYPE
{
  MTRANSFORM_NONE,
  MTRANSFORM_NEGATE
};

//! @internal
//!
//! @brief Function type.
enum MFUNCTION_TYPE
{
  MFUNCTION_CUSTOM = 0,

  // Min/Max.
  MFUNCTION_MIN,
  MFUNCTION_MAX,
  MFUNCTION_AVG,

  // Rounding.
  MFUNCTION_CEIL,
  MFUNCTION_FLOOR,
  MFUNCTION_ROUND,

  // Abs.
  MFUNCTION_ABS,

  // Reciprocal.
  MFUNCTION_RECIPROCAL,

  // Sqrt/Pow.
  MFUNCTION_SQRT,
  MFUNCTION_POW,

  // Log.
  MFUNCTION_LOG,
  MFUNCTION_LOG10,

  // Trigonometric.
  MFUNCTION_SIN,
  MFUNCTION_COS,
  MFUNCTION_TAN,

  MFUNCTION_SINH,
  MFUNCTION_COSH,
  MFUNCTION_TANH,

  MFUNCTION_ASIN,
  MFUNCTION_ACOS,
  MFUNCTION_ATAN,
  MFUNCTION_ATAN2
};

//! @internal
//!
//! @brief Variable type.
enum MVARIABLE_TYPE
{
  MVARIABLE_CONSTANT = 0,
  MVARIABLE_READ_ONLY = 1,
  MVARIABLE_READ_WRITE = 2
};

// ============================================================================
// [MathPresso::ASTElement]
// ============================================================================

class MATHPRESSO_HIDDEN ASTElement
{
protected:
  //! @brief Parent element.
  ASTElement* _parent;
  //! @brief Element type.
  uint _elementType;
  //! @brief Element id, unique per @ref Expression.
  uint _elementId;

public:
  ASTElement(uint elementId, uint elementType);
  virtual ~ASTElement() = 0;

  //! @brief Get whether this element is constant expression.
  //!
  //! Used by optimizer to evaluate all constant expressions first.
  virtual bool isConstant() const = 0;

  //! @brief Get array of children elements.
  virtual ASTElement** getChildrenElements() const = 0;

  //! @brief Get count of children elements.
  virtual size_t getChildrenCount() const = 0;

  //! @brief Replace child element @a child by @a element.
  virtual bool replaceChild(ASTElement* child, ASTElement* element);

  //! @brief Evaluate this element.
  virtual mreal_t evaluate(void* data) const = 0;

  //! @brief Get the parent element.
  inline ASTElement* & getParent() { return _parent; }

  //! @brief Get the element type.
  inline uint getElementId() const { return _elementId; }

  //! @brief Get the element type.
  inline uint getElementType() const { return _elementType; }
};

// ============================================================================
// [MathPresso::ASTBlock]
// ============================================================================

class MATHPRESSO_HIDDEN ASTBlock : public ASTElement
{
protected:
  Vector<ASTElement*> _elements;

public:
  ASTBlock(uint elementId);
  virtual ~ASTBlock();

  virtual bool isConstant() const;
  virtual ASTElement** getChildrenElements() const;
  virtual Vector<ASTElement *> & getChildrenVector();
  virtual size_t getChildrenCount() const;
  virtual mreal_t evaluate(void* data) const;
};

// ============================================================================
// [MathPresso::ASTNode]
// ============================================================================

class MATHPRESSO_HIDDEN ASTNode : public ASTElement
{
protected:
  union
  {
    struct
    {
      ASTElement* _left;
      ASTElement* _right;
    };
    struct
    {
      ASTElement* _elements[2];
    };
  };

public:
  ASTNode(uint elementId, uint elementType);
  virtual ~ASTNode();

  virtual bool isConstant() const;
  virtual ASTElement** getChildrenElements() const;
  virtual size_t getChildrenCount() const;

  inline ASTElement* getLeft() const { return _left; }
  inline ASTElement* getRight() const { return _right; }

  inline void setLeft(ASTElement* element) { _left = element; element->getParent() = this; }
  inline void setRight(ASTElement* element) { _right = element; element->getParent() = this; }
};

// ============================================================================
// [MathPresso::ASTConstant]
// ============================================================================

class MATHPRESSO_HIDDEN ASTConstant : public ASTElement
{
protected:
  mreal_t _value;

public:
  ASTConstant(uint elementId, mreal_t val);
  virtual ~ASTConstant();

  virtual bool isConstant() const;
  virtual ASTElement** getChildrenElements() const;
  virtual size_t getChildrenCount() const;
  virtual bool replaceChild(ASTElement* child, ASTElement* element);
  virtual mreal_t evaluate(void* data) const;

  inline mreal_t getValue() const { return _value; }
  inline void setValue(mreal_t value) { _value = value; }
};

// ============================================================================
// [MathPresso::ASTVariable]
// ============================================================================

class MATHPRESSO_HIDDEN ASTVariable : public ASTElement
{
protected:
  const Variable* _variable;

public:
  ASTVariable(uint elementId, const Variable* variable);
  virtual ~ASTVariable();

  virtual bool isConstant() const;
  virtual ASTElement** getChildrenElements() const;
  virtual size_t getChildrenCount() const;
  virtual bool replaceChild(ASTElement* child, ASTElement* element);
  virtual mreal_t evaluate(void* data) const;

  inline const Variable* getVariable() const { return _variable; }
  inline int getOffset() const { return _variable->v.offset; }
};

// ============================================================================
// [MathPresso::ASTOperator]
// ============================================================================

class MATHPRESSO_HIDDEN ASTOperator : public ASTNode
{
protected:
  uint _operatorType;

public:

  ASTOperator(uint elementId, uint operatorType);
  virtual ~ASTOperator();

  virtual mreal_t evaluate(void* data) const;

  inline uint getOperatorType() const { return _operatorType; }
  inline void setOperatorType(int value) { _operatorType = value; }

};

// ============================================================================
// [MathPresso::ASTCall]
// ============================================================================

class MATHPRESSO_HIDDEN ASTCall : public ASTElement
{
protected:
  Function* _function;
  Vector<ASTElement*> _arguments;

public:
  ASTCall(uint elementId, Function* function);
  virtual ~ASTCall();

  virtual bool isConstant() const;
  virtual ASTElement** getChildrenElements() const;
  virtual size_t getChildrenCount() const;
  virtual mreal_t evaluate(void* data) const;

  inline Function* getFunction() const { return _function; }

  inline Vector<ASTElement*>& getArguments() { return _arguments; }
  inline const Vector<ASTElement*>& getArguments() const { return _arguments; }

  inline bool swapArguments(Vector<ASTElement*>& other) { return _arguments.swap(other); }
};

// ============================================================================
// [MathPresso::ASTTransform]
// ============================================================================

class MATHPRESSO_HIDDEN ASTTransform : public ASTElement
{
protected:
  ASTElement* _child;
  uint _transformType;

public:
  ASTTransform(uint elementId);
  virtual ~ASTTransform();

  virtual bool isConstant() const;
  virtual ASTElement** getChildrenElements() const;
  virtual size_t getChildrenCount() const;
  virtual mreal_t evaluate(void* data) const;

  inline ASTElement* getChild() const { return _child; }
  inline virtual void setChild(ASTElement* element) { _child = element; element->getParent() = this; }

  inline uint getTransformType() const { return _transformType; }
  inline void setTransformType(uint transformType) { _transformType = transformType; }
};

} // MathPresso namespace

#endif // _MATHPRESSO_AST_P_H
