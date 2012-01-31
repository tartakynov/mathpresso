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
#include "MathPresso_JIT_p.h"
#include "MathPresso_Util_p.h"

#include <AsmJit/Compiler.h>
#include <AsmJit/Logger.h>
#include <AsmJit/MemoryManager.h>
#include <AsmJit/Util.h>

namespace MathPresso {

// ============================================================================
// [MathPresso::JitVar]
// ============================================================================

struct MATHPRESSO_HIDDEN JitVar
{
  inline JitVar() :
    op(),
    flags(FLAG_NONE)
  {
  }

  inline JitVar(AsmJit::Operand op, uint32_t flags) :
    op(op),
    flags(flags)
  {
  }

  inline JitVar(const JitVar& other) :
    op(other.op),
    flags(other.flags)
  {
  }

  inline ~JitVar()
  {
  }

  // Operator Overload.

  inline const JitVar& operator=(const JitVar& other)
  {
    op = other.op;
    flags = other.flags;
    return *this;
  }

  // inline bool operator==(const JitVar& other) { return op == other.op; }
  // inline bool operator!=(const JitVar& other) { return op != other.op; }

  // Swap.

  void swapWith(JitVar& other)
  {
    JitVar t(*this);
    *this = other;
    other = t;
  }

  // Operand management.

  inline AsmJit::Operand& getOperand() { return op; }
  inline AsmJit::XMMVar& getXmm() { return *reinterpret_cast<AsmJit::XMMVar*>(&op); }
  inline AsmJit::Mem& getMem() { return *reinterpret_cast<AsmJit::Mem*>(&op); }

  inline const AsmJit::Operand& getOperand() const { return op; }
  inline const AsmJit::XMMVar& getXmm() const { return *reinterpret_cast<const AsmJit::XMMVar*>(&op); }
  inline const AsmJit::Mem& getMem() const { return *reinterpret_cast<const AsmJit::Mem*>(&op); }

  inline bool isXmm() const { return op.isRegType(AsmJit::REG_TYPE_XMM); }
  inline bool isMem() const { return op.isMem(); }

  // Flags.

  inline bool isRO() const { return (flags & FLAG_RO) != 0; }

  // Members.

  AsmJit::Operand op;
  uint32_t flags;

  enum FLAGS
  {
    FLAG_NONE = 0,
    FLAG_RO = 1
  };
};

// ============================================================================
// [MathPresso::JitConst]
// ============================================================================

struct MATHPRESSO_HIDDEN JitConst
{
  inline JitConst(const JitVar& var, int32_t value) : var(var), value(value) {}

  JitVar var;
  int32_t value;
};

// ============================================================================
// [MathPresso::JitLogger]
// ============================================================================

struct MATHPRESSO_HIDDEN JitLogger : public AsmJit::Logger
{
  JitLogger() ASMJIT_NOTHROW;
  virtual ~JitLogger() ASMJIT_NOTHROW;
  virtual void logString(const char* buf, sysuint_t len) ASMJIT_NOTHROW;

  inline StringBuilder& getStringBuilder() { return _sb; }

  StringBuilder _sb;
};

JitLogger::JitLogger() ASMJIT_NOTHROW {}
JitLogger::~JitLogger() ASMJIT_NOTHROW {}

void JitLogger::logString(const char* buf, sysuint_t len) ASMJIT_NOTHROW
{
  _sb.appendString(buf, len);
}

// ============================================================================
// [MathPresso::JitCompiler]
// ============================================================================

struct MATHPRESSO_HIDDEN JitCompiler
{
  JitCompiler(WorkContext& ctx, AsmJit::Compiler* c);
  ~JitCompiler();

  // Function Generator.

  void beginFunction();
  void endFunction();
  MEvalFunc make();

  // Variable Management.

  JitVar copyVar(const JitVar& other);
  JitVar writableVar(const JitVar& other);
  JitVar registerVar(const JitVar& other);

  // Analyzer.

  // Compiler.

  void doTree(ASTElement* tree);
  JitVar doElement(ASTElement* element);
  JitVar doBlock(ASTBlock* element);
  JitVar doConstant(ASTConstant* element);
  JitVar doVariable(ASTVariable* element);
  JitVar doOperator(ASTOperator* element);
  JitVar doCall(ASTCall* element);
  JitVar doTransform(ASTTransform* element);

  // Constants.

  JitVar getConstantI32(int32_t value);
  JitVar getConstantF32(float value);

  // Members.

  WorkContext& ctx;
  AsmJit::Compiler* c;

  AsmJit::GPVar resultAddress;
  AsmJit::GPVar variablesAddress;
  AsmJit::GPVar dataAddress;

  AsmJit::Emittable* bodyEmittable;
  AsmJit::PodVector<JitConst> constVariables;

  AsmJit::Label dataLabel;
  AsmJit::Buffer dataBuffer;
};

JitCompiler::JitCompiler(WorkContext& ctx, AsmJit::Compiler* c) :
  ctx(ctx),
  c(c)
{
}

JitCompiler::~JitCompiler()
{
}

void JitCompiler::beginFunction()
{
  // Declare function.
  c->newFunction(
    AsmJit::CALL_CONV_DEFAULT,
    AsmJit::FunctionBuilder3<AsmJit::Void, const void*, mreal_t*, mreal_t*>());
  c->getFunction()->setHint(AsmJit::FUNCTION_HINT_NAKED, true);

  resultAddress = c->argGP(1);
  variablesAddress = c->argGP(2);
  dataAddress = c->newGP(AsmJit::VARIABLE_TYPE_GPN, "data");

  c->setPriority(variablesAddress, 1);
  c->setPriority(dataAddress, 2);

  bodyEmittable = c->getCurrentEmittable();

  // Data and constants.
  dataLabel = c->newLabel();
}

void JitCompiler::endFunction()
{
  c->endFunction();
  c->bind(dataLabel);
  c->embed(dataBuffer.getData(), dataBuffer.getOffset());
}

MEvalFunc JitCompiler::make()
{
  return AsmJit::function_cast<MEvalFunc>(c->make());
}

JitVar JitCompiler::copyVar(const JitVar& other)
{
  JitVar v(c->newXMM(AsmJit::VARIABLE_TYPE_XMM_1F), JitVar::FLAG_NONE);
  c->emit(AsmJit::INST_MOVSS, v.getXmm(), other.getOperand());
  return v;
}

JitVar JitCompiler::writableVar(const JitVar& other)
{
  if (other.isMem() || other.isRO())
    return copyVar(other);
  else
    return other;
}

JitVar JitCompiler::registerVar(const JitVar& other)
{
  if (other.isMem())
    return copyVar(other);
  else
    return other;
}

void JitCompiler::doTree(ASTElement* tree)
{
  JitVar result = registerVar(doElement(tree));
  c->movss(ptr(resultAddress), result.getXmm());
}

JitVar JitCompiler::doElement(ASTElement* element)
{
  switch (element->getElementType())
  {
    case MELEMENT_BLOCK:
      return doBlock(reinterpret_cast<ASTBlock*>(element));
    case MELEMENT_CONSTANT:
      return doConstant(reinterpret_cast<ASTConstant*>(element));
    case MELEMENT_VARIABLE:
      return doVariable(reinterpret_cast<ASTVariable*>(element));
    case MELEMENT_OPERATOR:
      return doOperator(reinterpret_cast<ASTOperator*>(element));
    case MELEMENT_CALL:
      return doCall(reinterpret_cast<ASTCall*>(element));
    case MELEMENT_TRANSFORM:
      return doTransform(reinterpret_cast<ASTTransform*>(element));
    default:
      MP_ASSERT_NOT_REACHED();
      return JitVar();
  }
}

JitVar JitCompiler::doBlock(ASTBlock* element)
{
  JitVar result;

  size_t i, len = element->_elements.getLength();
  for (i = 0; i < len; i++)
  {
    result = doElement(element->_elements[i]);
  }

  return result;
}

JitVar JitCompiler::doConstant(ASTConstant* element)
{
  return getConstantF32(element->getValue());
}

JitVar JitCompiler::doVariable(ASTVariable* element)
{
  return JitVar(ptr(variablesAddress, (sysint_t)element->getOffset()), JitVar::FLAG_RO);
}

JitVar JitCompiler::doOperator(ASTOperator* element)
{
  uint operatorType = element->getOperatorType();
  JitVar vl;
  JitVar vr;

  ASTElement* left  = element->getLeft();
  ASTElement* right = element->getRight();

  if (operatorType == MOPERATOR_ASSIGN)
  {
    ASTVariable* varNode = reinterpret_cast<ASTVariable*>(left);
    MP_ASSERT(varNode->getElementType() == MELEMENT_VARIABLE);

    vr = registerVar(doElement(right));
    c->emit(AsmJit::INST_MOVSS, ptr(variablesAddress, (sysint_t)varNode->getOffset()), vr.getOperand());
    return vr;
  }

  if (left->getElementType() == MELEMENT_VARIABLE && right->getElementType() == MELEMENT_VARIABLE &&
      reinterpret_cast<ASTVariable*>(left)->getOffset() == reinterpret_cast<ASTVariable*>(right)->getOffset())
  {
    // vl OP vr, emit:
    //
    //   LOAD vl
    //   INST vl, vl
    vl = writableVar(doElement(element->getLeft()));
    vr = vl;
  }
  else
  {
    // vl OP vr, emit:
    //
    //   LOAD vl
    //   LOAD vr
    //   INST vl, vr OR vr, vl

    vl = doElement(element->getLeft());
    vr = doElement(element->getRight());

    // Commutativity (PLUS and MUL operators).
    if (operatorType == MOPERATOR_PLUS || operatorType == MOPERATOR_MUL)
    {
      if (vl.isRO() && !vr.isRO()) vl.swapWith(vr);
    }

    vl = writableVar(vl);
  }

  switch (operatorType)
  {
    case MOPERATOR_PLUS:
      c->emit(AsmJit::INST_ADDSS, vl.getOperand(), vr.getOperand());
      return vl;
    case MOPERATOR_MINUS:
      c->emit(AsmJit::INST_SUBSS, vl.getOperand(), vr.getOperand());
      return vl;
    case MOPERATOR_MUL:
      c->emit(AsmJit::INST_MULSS, vl.getOperand(), vr.getOperand());
      return vl;
    case MOPERATOR_DIV:
      c->emit(AsmJit::INST_DIVSS, vl.getOperand(), vr.getOperand());
      return vl;
    case MOPERATOR_MOD:
      // TODO: Modulo.
      return vl;
    default:
      MP_ASSERT_NOT_REACHED();
      return vl;
  }
}

JitVar JitCompiler::doCall(ASTCall* element)
{
  const Vector<ASTElement*>& arguments = element->getArguments();
  size_t i, len = arguments.getLength();
  
  Function* fn = element->getFunction();
  int funcId = fn->getFunctionId();

  switch (funcId)
  {
    // Intrinsics.
    case MFUNCTION_MIN:
    case MFUNCTION_MAX:
    case MFUNCTION_AVG:
    {
      MP_ASSERT(len == 2);

      JitVar vl(doElement(arguments[0]));
      JitVar vr(doElement(arguments[1]));
      if (vl.isRO() && !vr.isRO()) vl.swapWith(vr);

      vl = writableVar(vl);

      switch (funcId)
      {
        case MFUNCTION_MIN:
          c->emit(AsmJit::INST_MINSS, vl.getOperand(), vr.getOperand());
          break;
        case MFUNCTION_MAX:
          c->emit(AsmJit::INST_MAXSS, vl.getOperand(), vr.getOperand());
          break;
        case MFUNCTION_AVG:
          c->emit(AsmJit::INST_ADDSS, vl.getOperand(), vr.getOperand());
          c->emit(AsmJit::INST_MULSS, vl.getOperand(), getConstantF32(0.5f).getOperand());
          break;
      }
      return vl;
    }

    case MFUNCTION_ABS:
    case MFUNCTION_RECIPROCAL:
    case MFUNCTION_SQRT:
    {
      MP_ASSERT(len == 1);
      JitVar vl(doElement(arguments[0]));

      vl = writableVar(vl);

      switch (funcId)
      {
        case MFUNCTION_ABS:
          c->emit(AsmJit::INST_ANDPS, vl.getOperand(), registerVar(getConstantI32(0x8000000)).getOperand());
          break;
        case MFUNCTION_RECIPROCAL:
          c->emit(AsmJit::INST_RCPSS, vl.getOperand(), vl.getOperand());
          break;
        case MFUNCTION_SQRT:
          c->emit(AsmJit::INST_SQRTSS, vl.getOperand(), vl.getOperand());
          break;
      }

      return vl;
    }

    // Function call.
    default:
    {
      AsmJit::XMMVar vars[8];

      for (i = 0; i < len; i++)
      {
        JitVar tmp = doElement(arguments[i]);

        if (tmp.isXmm())
        {
          vars[i] = tmp.getXmm();
        }
        else
        {
          vars[i] = c->newXMM(AsmJit::VARIABLE_TYPE_XMM_1F);
          c->emit(AsmJit::INST_MOVSS, vars[i], tmp.getOperand());
        }
      }

      // Use function builder to build a function prototype.
      AsmJit::FunctionBuilderX builder;
      for (i = 0; i < len; i++)
	  {
        builder.addArgument<mreal_t>();
      }
      builder.setReturnValue<mreal_t>();

      // Create ECall emittable (function call context).
      AsmJit::ECall* ctx = c->call(element->getFunction()->getPtr());
      ctx->setPrototype(AsmJit::CALL_CONV_DEFAULT, builder);

      // Assign arguments.
      for (i = 0; i < len; i++) ctx->setArgument((uint)i, vars[i]);

      // Fetch return value.
      JitVar result(c->newXMM(AsmJit::VARIABLE_TYPE_XMM_1F), JitVar::FLAG_NONE);
      ctx->setReturn(result.getXmm());
      return result;
    }
  }
}

JitVar JitCompiler::doTransform(ASTTransform* element)
{
  uint transformType = element->getTransformType();
  JitVar var = doElement(element->getChild());

  if (transformType != MTRANSFORM_NONE) var = writableVar(var);

  switch (transformType)
  {
    case MTRANSFORM_NONE:
      break;
    case MTRANSFORM_NEGATE:
    {
      AsmJit::XMMVar t(c->newXMM());
      c->emit(AsmJit::INST_MOVSS, t, getConstantI32(0x80000000).getOperand());
      c->emit(AsmJit::INST_XORPS, var.getOperand(), t);
      break;
    }
  }

  return var;
}

//! @internal
union I32FPUnion
{
  //! @brief 32 bit signed integer value.
  int32_t i32;
  //! @brief 32 bit SP-FP value.
  float f32;
};

JitVar JitCompiler::getConstantI32(int32_t value)
{
  size_t i = 0;
  size_t length = constVariables.getLength();

  if (length == 0)
  {
    AsmJit::Emittable* old = c->setCurrentEmittable(bodyEmittable);
    c->lea(dataAddress, ptr(dataLabel));
    if (old != bodyEmittable) c->setCurrentEmittable(old);
  }
  else
  {
    do {
      if (constVariables[i].value == value) return constVariables[i].var;
    } while (++i < length);
  }

  JitVar var(ptr(dataAddress, (sysint_t)i * sizeof(int32_t)), JitVar::FLAG_RO);

  constVariables.append(JitConst(var, value));
  dataBuffer.ensureSpace();
  dataBuffer.emitDWord((uint32_t)value);

  return var;
}

JitVar JitCompiler::getConstantF32(float value)
{
  I32FPUnion u;
  u.f32 = value;
  return getConstantI32(u.i32);
}

MEvalFunc mpCompileFunction(WorkContext& ctx, ASTElement* tree)
{
  bool enableLogger = true;
  AsmJit::Compiler c;

  JitLogger logger;
  if (enableLogger)
  {
    logger.setLogBinary(true);
    c.setLogger(&logger);
  }

  JitCompiler jitCompiler(ctx, &c);

  jitCompiler.beginFunction();
  jitCompiler.doTree(tree);
  jitCompiler.endFunction();

  MEvalFunc fn = jitCompiler.make();

  if (enableLogger)
  {
    char* log = logger.getStringBuilder().toString();
    printf("%s\n", log);
    free(log);
  }

  return fn;
}

void mpFreeFunction(void* fn)
{
  AsmJit::MemoryManager::getGlobal()->free((void*)fn);
}

} // MathPresso namespace
