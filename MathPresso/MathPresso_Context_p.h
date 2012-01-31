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

#ifndef _MATHPRESSO_CONTEXT_P_H
#define _MATHPRESSO_CONTEXT_P_H

#include "MathPresso.h"
#include "MathPresso_Util_p.h"

namespace MathPresso {

struct ASTElement;

// ============================================================================
// [MathPresso::MFunc]
// ============================================================================

typedef mreal_t (*MFunc_Ret_F_ARG0)(void);
typedef mreal_t (*MFunc_Ret_F_ARG1)(mreal_t);
typedef mreal_t (*MFunc_Ret_F_ARG2)(mreal_t, mreal_t);
typedef mreal_t (*MFunc_Ret_F_ARG3)(mreal_t, mreal_t, mreal_t);
typedef mreal_t (*MFunc_Ret_F_ARG4)(mreal_t, mreal_t, mreal_t, mreal_t);
typedef mreal_t (*MFunc_Ret_F_ARG5)(mreal_t, mreal_t, mreal_t, mreal_t, mreal_t);
typedef mreal_t (*MFunc_Ret_F_ARG6)(mreal_t, mreal_t, mreal_t, mreal_t, mreal_t, mreal_t);
typedef mreal_t (*MFunc_Ret_F_ARG7)(mreal_t, mreal_t, mreal_t, mreal_t, mreal_t, mreal_t, mreal_t);
typedef mreal_t (*MFunc_Ret_F_ARG8)(mreal_t, mreal_t, mreal_t, mreal_t, mreal_t, mreal_t, mreal_t, mreal_t);

// ============================================================================
// [MathPresso::Function]
// ============================================================================

struct Function
{
  inline Function(void* ptr, int prototype, int functionId)
  {
    this->ptr = ptr;
    this->prototype = prototype;
    this->functionId = functionId;
  }

  inline void* getPtr() const { return ptr; }
  inline int getPrototype() const { return prototype; }
  inline int getArguments() const { return prototype & 0xFF; }
  inline int getFunctionId() const { return functionId; }

  void* ptr;
  int prototype;
  int functionId;
};

// ============================================================================
// [MathPresso::Variable]
// ============================================================================

struct Variable
{
  inline Variable(int type, mreal_t value)
  {
    this->type = type;
    this->c.value = value;
  }

  inline Variable(int type, int offset, int flags)
  {
    this->type = type;
    this->v.offset = offset;
    this->v.flags = flags;
  }

  int type;

  union
  {
    struct
    {
      mreal_t value;
    } c;

    struct
    {
      int offset;
      int flags;
    } v;
  };
};

// ============================================================================
// [MathPresso::ContextPrivate]
// ============================================================================

struct MATHPRESSO_HIDDEN ContextPrivate
{
  ContextPrivate();
  ~ContextPrivate();

  inline void addRef() { refCount.inc(); }
  inline void release() { if (refCount.dec()) delete this; }
  inline bool isDetached() { return refCount.get() == 1; }

  ContextPrivate* copy() const;

  Atomic refCount;

  Hash<Variable> variables;
  Hash<Function> functions;

private:
  // DISABLE COPY of ContextPrivate instance.
  ContextPrivate(const ContextPrivate& other);
  ContextPrivate& operator=(const ContextPrivate& other);
};

// ============================================================================
// [MathPresso::ExpressionPrivate]
// ============================================================================

struct ExpressionPrivate
{
  inline ExpressionPrivate() :
    ast(NULL),
    ctx(NULL)
  {
  }

  inline ~ExpressionPrivate()
  {
    MP_ASSERT(ast == NULL);
    MP_ASSERT(ctx == NULL);
  }

  ASTElement* ast;
  ContextPrivate* ctx;
};

// ============================================================================
// [MathPresso::WorkContext]
// ============================================================================

//! @internal
//!
//! @brief Simple class that is used to generate element IDs.
struct WorkContext
{
  WorkContext(const Context& ctx);
  ~WorkContext();

  //! @brief Get next id.
  inline uint genId() { return _id++; }

  //! @brief Context data.
  ContextPrivate* _ctx;

  //! @brief Current counter position.
  uint _id;
};

} // MathPresso namespace

#endif // _MATHPRESSO_CONTEXT_P_H
