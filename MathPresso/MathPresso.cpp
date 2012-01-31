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
#include "MathPresso_Context_p.h"
#include "MathPresso_Optimizer_p.h"
#include "MathPresso_Parser_p.h"
#include "MathPresso_Tokenizer_p.h"
#include "MathPresso_Util_p.h"

#include "MathPresso_DOT_p.h"
#include "MathPresso_JIT_p.h"

#include <math.h>
#include <string.h>

namespace MathPresso {

// ============================================================================
// [MathPresso::Context - Construction / Destruction]
// ============================================================================

Context::Context()
{
  _privateData = new(std::nothrow) ContextPrivate();
}

Context::Context(const Context& other)
{
  ContextPrivate* d = reinterpret_cast<ContextPrivate*>(other._privateData);
  if (d) d->addRef();
  _privateData = d;
}

Context::~Context()
{
  ContextPrivate* d = reinterpret_cast<ContextPrivate*>(_privateData);
  if (d) d->release();
}

// ============================================================================
// [MathPresso::Context - Private]
// ============================================================================

static mresult_t Context_addFunction(Context* self, const char* name, void* ptr, int prototype, int functionId = -1)
{
  ContextPrivate* d = reinterpret_cast<ContextPrivate*>(self->_privateData);
  if (d == NULL) return MRESULT_NO_MEMORY;

  size_t nlen = strlen(name);

  Function* fdata = d->functions.get(name, nlen);
  if (fdata && fdata->ptr == ptr && fdata->prototype == prototype) return MRESULT_OK;

  if (!d->isDetached())
  {
    d = d->copy();
    if (!d) return MRESULT_NO_MEMORY;

    reinterpret_cast<ContextPrivate*>(self->_privateData)->release();
    self->_privateData = d;
  }

  return d->functions.put(name, nlen, Function(ptr, prototype, functionId))
    ? MRESULT_OK
    : MRESULT_NO_MEMORY;
}

// ============================================================================
// [MathPresso::Context - Environment]
// ============================================================================

static float minf(float x, float y) { return x < y ? x : y; }
static float maxf(float x, float y) { return x > y ? x : y; }
static float avgf(float x, float y) { return (x + y) * 0.5f; }

static float roundf(float x) { return (float)( (int)((x < 0.0f) ? x - 0.5f : x + 0.5f) ); }
static float recipf(float x) { return 1.0f/x; }

#define MP_ADD_CONST(self, name, value) \
  do { \
    mresult_t __r = self->addConstant(name, (float)(value)); \
    if (__r != MRESULT_OK) return __r; \
  } while (0)

#define MP_ADD_FUNCTION(self, name, ptr, prototype, id) \
  do { \
    mresult_t __r = Context_addFunction(self, name, (void*)(ptr), prototype, id); \
    if (__r != MRESULT_OK) return __r; \
  } while (0)

static mresult_t Context_addEnvironment_Math(Context* self)
{
  // Constants.
  MP_ADD_CONST(self, "E", 2.7182818284590452354);
  MP_ADD_CONST(self, "PI", 3.14159265358979323846);

  // Functions.
  MP_ADD_FUNCTION(self, "min"       , minf   , MFUNC_F_ARG2 | MFUNC_EVAL, MFUNCTION_MIN);
  MP_ADD_FUNCTION(self, "max"       , maxf   , MFUNC_F_ARG2 | MFUNC_EVAL, MFUNCTION_MAX);
  MP_ADD_FUNCTION(self, "avg"       , avgf   , MFUNC_F_ARG2 | MFUNC_EVAL, MFUNCTION_AVG);

  MP_ADD_FUNCTION(self, "ceil"      , ceilf  , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_CEIL);
  MP_ADD_FUNCTION(self, "floor"     , floorf , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_FLOOR);
  MP_ADD_FUNCTION(self, "round"     , roundf , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_ROUND);

  MP_ADD_FUNCTION(self, "abs"       , fabsf  , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_ABS);

  MP_ADD_FUNCTION(self, "reciprocal", recipf , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_RECIPROCAL);

  MP_ADD_FUNCTION(self, "sqrt"      , sqrtf  , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_SQRT);
  MP_ADD_FUNCTION(self, "pow"       , powf   , MFUNC_F_ARG2 | MFUNC_EVAL, MFUNCTION_POW);

  MP_ADD_FUNCTION(self, "log"       , logf   , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_LOG);
  MP_ADD_FUNCTION(self, "log10"     , log10f , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_LOG10);

  MP_ADD_FUNCTION(self, "sin"       , sinf   , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_SIN);
  MP_ADD_FUNCTION(self, "cos"       , cosf   , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_COS);
  MP_ADD_FUNCTION(self, "tan"       , tanf   , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_TAN);

  MP_ADD_FUNCTION(self, "sinh"      , sinhf  , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_SINH);
  MP_ADD_FUNCTION(self, "cosh"      , coshf  , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_COSH);
  MP_ADD_FUNCTION(self, "tanh"      , tanhf  , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_TANH);

  MP_ADD_FUNCTION(self, "asin"      , asinf  , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_ASIN);
  MP_ADD_FUNCTION(self, "acos"      , acosf  , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_ACOS);
  MP_ADD_FUNCTION(self, "atan"      , atanf  , MFUNC_F_ARG1 | MFUNC_EVAL, MFUNCTION_ATAN);
  MP_ADD_FUNCTION(self, "atan2"     , atan2f , MFUNC_F_ARG2 | MFUNC_EVAL, MFUNCTION_ATAN2);

  return MRESULT_OK;
}

mresult_t Context::addEnvironment(int environmentId)
{
  switch (environmentId)
  {
    case MENVIRONMENT_MATH:
    {
      return Context_addEnvironment_Math(this);
    }

    case MENVIRONMENT_ALL:
    {
      mresult_t result;
      for (int i = 1; i < _MENVIRONMENT_COUNT; i++)
      {
        if ((result = addEnvironment(i)) != MRESULT_OK) return result;
      }
      return MRESULT_OK;
    }

    default:
      return MRESULT_INVALID_ARGUMENT;
  }
}

// ============================================================================
// [MathPresso::Context - Function]
// ============================================================================

mresult_t Context::addFunction(const char* name, void* ptr, int prototype)
{
  return Context_addFunction(this, name, ptr, prototype);
}

// ============================================================================
// [MathPresso::Context - Constant]
// ============================================================================

mresult_t Context::addConstant(const char* name, float value)
{
  ContextPrivate* d = reinterpret_cast<ContextPrivate*>(_privateData);
  if (d == NULL) return MRESULT_NO_MEMORY;

  size_t nlen = strlen(name);

  Variable* variable = d->variables.get(name, nlen);
  if (variable && variable->type == MVARIABLE_CONSTANT && 
                  variable->c.value == value)
  {
    return MRESULT_OK;
  }

  if (!d->isDetached())
  {
    d = d->copy();
    if (!d) return MRESULT_NO_MEMORY;

    reinterpret_cast<ContextPrivate*>(_privateData)->release();
    _privateData = d;
  }

  return d->variables.put(name, nlen, Variable(MVARIABLE_CONSTANT, value))
    ? MRESULT_OK
    : MRESULT_NO_MEMORY;
}

// ============================================================================
// [MathPresso::Context - Variable]
// ============================================================================

mresult_t Context::addVariable(const char* name, int offset, int flags)
{
  ContextPrivate* d = reinterpret_cast<ContextPrivate*>(_privateData);
  if (d == NULL) return MRESULT_NO_MEMORY;

  size_t nlen = strlen(name);
  int type = (flags & MVAR_READ_ONLY) ? MVARIABLE_READ_ONLY : MVARIABLE_READ_WRITE;

  Variable* variable = d->variables.get(name, nlen);
  if (variable && variable->type == type && 
                  variable->v.offset == offset && 
                  variable->v.flags == flags)
  {
    return MRESULT_OK;
  }

  if (!d->isDetached())
  {
    d = d->copy();
    if (!d) return MRESULT_NO_MEMORY;

    reinterpret_cast<ContextPrivate*>(_privateData)->release();
    _privateData = d;
  }

  return d->variables.put(name, nlen, Variable(type, offset, flags))
    ? MRESULT_OK
    : MRESULT_NO_MEMORY;
}

// ============================================================================
// [MathPresso::Context - Symbol]
// ============================================================================

mresult_t Context::delSymbol(const char* name)
{
  ContextPrivate* d = reinterpret_cast<ContextPrivate*>(_privateData);
  if (d == NULL) return MRESULT_NO_MEMORY;

  size_t nlen = strlen(name);
  if (!d->variables.contains(name, nlen) &&
      !d->functions.contains(name, nlen))
  {
    return MRESULT_OK;
  }

  if (!d->isDetached())
  {
    d = d->copy();
    if (!d) return MRESULT_NO_MEMORY;

    reinterpret_cast<ContextPrivate*>(_privateData)->release();
    _privateData = d;
  }

  d->variables.remove(name, nlen);
  d->functions.remove(name, nlen);
  return MRESULT_OK;
}

// ============================================================================
// [MathPresso::Context - Clear]
// ============================================================================

mresult_t Context::clear()
{
  ContextPrivate* d = reinterpret_cast<ContextPrivate*>(_privateData);
  if (d == NULL) return MRESULT_NO_MEMORY;

  if (!d->isDetached())
  {
    d = new(std::nothrow) ContextPrivate();
    if (!d) return MRESULT_NO_MEMORY;

    reinterpret_cast<ContextPrivate*>(_privateData)->release();
    _privateData = d;
  }
  else
  {
    d->variables.clear();
    d->functions.clear();
  }

  return MRESULT_OK;
}

// ============================================================================
// [MathPresso::Context - Operator Overload]
// ============================================================================

Context& Context::operator=(const Context& other)
{
  ContextPrivate* newp = reinterpret_cast<ContextPrivate*>(other._privateData);
  if (newp) newp->addRef();

  ContextPrivate* oldp = reinterpret_cast<ContextPrivate*>(_privateData);
  if (oldp) oldp->release();

  _privateData = newp;
  return *this;
}

// ============================================================================
// [MathPresso::Eval Entry-Points]
// ============================================================================

static void mEvalDummy(const void*, mreal_t* result, void*)
{
  *result = 0.0f;
}

static void mEvalExpression(const void* _p, mreal_t* result, void* data)
{
  const ExpressionPrivate* p = reinterpret_cast<const ExpressionPrivate*>(_p);

  MP_ASSERT(p->ast != NULL);
  *result = p->ast->evaluate(data);
}

// ============================================================================
// [MathPresso::Expression - Construction / Destruction]
// ============================================================================

Expression::Expression() :
  _privateData(NULL),
  _evaluate(mEvalDummy)
{
  _privateData = new(std::nothrow) ExpressionPrivate();
}

Expression::~Expression()
{
  free();
  if (_privateData) delete reinterpret_cast<ExpressionPrivate*>(_privateData);
}

// ============================================================================
// [MathPresso::Expression - Create / Free]
// ============================================================================

mresult_t Expression::create(const Context& ectx, const char* expression, int options)
{
  if (_privateData == NULL) return MRESULT_NO_MEMORY;

  ExpressionPrivate* p = reinterpret_cast<ExpressionPrivate*>(_privateData);
  WorkContext ctx(ectx);

  // Destroy previous expression and prepare for error state (if something 
  // will fail).
  free();

  // Parse the expression.
  ExpressionParser parser(ctx, expression, strlen(expression));

  ASTElement* ast = NULL;
  int result = parser.parse(&ast);
  if (result != MRESULT_OK) return result;

  // If something failed, report it, but don't continue.
  if (ast == NULL) return MRESULT_NO_EXPRESSION;

  // Simplify the expression, evaluating all constant parts.
  {
    Optimizer simplifier(ctx);
    ast = simplifier.doNode(ast);
  }

  // Compile using JIT compiler if enabled.
  if (options & MOPTION_NO_JIT)
    _evaluate = NULL;
  else
    _evaluate = mpCompileFunction(ctx, ast);

/*
  {
    char* d = mpCreateDot(ctx, ast);
    printf(f, "%s\n", d);
    ::free((void*)d);
  }
*/

  // Fallback to evaluation if JIT compiling failed or not enabled.
  if (_evaluate == NULL)
  {
    _evaluate = mEvalExpression;
    p->ast = ast;
  }
  else
  {
    delete ast;
  }

  p->ctx = ctx._ctx;
  p->ctx->addRef();

  // All fine...
  return MRESULT_OK;
}

void Expression::free()
{
  if (_privateData == NULL) return;
  ExpressionPrivate* p = reinterpret_cast<ExpressionPrivate*>(_privateData);

  if (_evaluate != mEvalDummy &&
      _evaluate != mEvalExpression)
  {
    // Allocated by JIT memory manager, free it.
    mpFreeFunction((void*)_evaluate);
  }

  // Set evaluate to dummy function so it will not crash when called through
  // Expression::evaluate().
  _evaluate = mEvalDummy;

  if (p->ast)
  {
    delete p->ast;
    p->ast = NULL;
  }

  // Release context.
  if (p->ctx)
  {
    p->ctx->release();
    p->ctx = NULL;
  }
}

} // MathPresso namespace
