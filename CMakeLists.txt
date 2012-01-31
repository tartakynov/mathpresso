# Reguire minimum version of CMake.
CMake_Minimum_Required(VERSION 2.6)

# MathPresso project - Need to use both C and C++.
Project(MathPresso C CXX)

# MathPresso.
Set(MATHPRESSO_DIR ${CMAKE_SOURCE_DIR})
Include_Directories(${MATHPRESSO_DIR})

Set(MATHPRESSO_SOURCES
  MathPresso/MathPresso.cpp
  MathPresso/MathPresso_AST.cpp
  MathPresso/MathPresso_AST_p.h
  MathPresso/MathPresso_Context.cpp
  MathPresso/MathPresso_Context_p.h
  MathPresso/MathPresso_DOT.cpp
  MathPresso/MathPresso_DOT_p.h
  MathPresso/MathPresso_JIT.cpp
  MathPresso/MathPresso_JIT_p.h
  MathPresso/MathPresso_Optimizer.cpp
  MathPresso/MathPresso_Optimizer_p.h
  MathPresso/MathPresso_Parser.cpp
  MathPresso/MathPresso_Parser_p.h
  MathPresso/MathPresso_Tokenizer.cpp
  MathPresso/MathPresso_Tokenizer_p.h
  MathPresso/MathPresso_Util.cpp
  MathPresso/MathPresso_Util_p.h
)

Set(MATHPRESSO_HEADERS
  MathPresso/MathPresso.h
)

# AsmJit.
Find_Library(ASMJIT_LIBRARY NAMES "AsmJit")
If(ASMJIT_LIBRARY)
  Set(ASMJIT_FOUND TRUE)
  If(NOT ASMJIT_INCLUDE_DIR)
    Set(ASMJIT_INCLUDE_FILE AsmJit/AsmJit.h)
    Find_File(ASMJIT_INCLUDE_FILE_PATH NAMES ${ASMJIT_INCLUDE_FILE})
    String(REPLACE ${ASMJIT_INCLUDE_FILE} "" ASMJIT_INCLUDE_DIR_ ${ASMJIT_INCLUDE_FILE_PATH})
    Unset(ASMJIT_INCLUDE_FILE)
    Set(ASMJIT_INCLUDE_DIR ${ASMJIT_INCLUDE_DIR_})
  EndIf(NOT ASMJIT_INCLUDE_DIR)
EndIf(ASMJIT_LIBRARY)

If(ASMJIT_FOUND)
  Message(STATUS "AsmJit: Lib=${ASMJIT_LIBRARY}, Include=${ASMJIT_INCLUDE_DIR}")
  Include_Directories(${ASMJIT_INCLUDE_DIR})
Else(ASMJIT_FOUND)
  Message(FATAL_ERROR "AsmJit: NOT FOUND, see http://code.google.com/p/asmjit/")
EndIf(ASMJIT_FOUND)

# Binary.
Add_Executable(evaluator Test/evaluator.cpp ${MATHPRESSO_SOURCES} ${MATHPRESSO_HEADERS})
Add_Executable(exptest   Test/exptest.cpp   ${MATHPRESSO_SOURCES} ${MATHPRESSO_HEADERS})

Target_Link_Libraries(evaluator ${ASMJIT_LIBRARY})
Target_Link_Libraries(exptest   ${ASMJIT_LIBRARY})
