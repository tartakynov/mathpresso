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

#include <MathPresso/MathPresso.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct TestExpression
{
  const char* expression;
  MathPresso::mreal_t expected;
};

#define TABLE_SIZE(table) \
  (sizeof(table) / sizeof(table[0]))

#define TEST_EXPRESSION(expression) \
  { #expression, (MathPresso::mreal_t)(expression) }

int main(int argc, char* argv[])
{
  MathPresso::mreal_t x = 5.1f;
  MathPresso::mreal_t y = 6.7f;
  MathPresso::mreal_t z = 9.9f;
  MathPresso::mreal_t PI = 3.14159265358979323846f;

  MathPresso::Context ctx;
  MathPresso::Expression e0;
  MathPresso::Expression e1;

  ctx.addEnvironment(MathPresso::MENVIRONMENT_ALL);
  ctx.addVariable("x", 0 * sizeof(MathPresso::mreal_t));
  ctx.addVariable("y", 1 * sizeof(MathPresso::mreal_t));
  ctx.addVariable("z", 2 * sizeof(MathPresso::mreal_t));

  MathPresso::mreal_t variables[] = { x, y, z };

  TestExpression tests[] = {
    TEST_EXPRESSION( (x+y) ),
    TEST_EXPRESSION( -x ),
    TEST_EXPRESSION( -(x+y) ),
    TEST_EXPRESSION( -x ),
    TEST_EXPRESSION( -1 + x ),
    TEST_EXPRESSION( -(-(-1)) ),
    TEST_EXPRESSION( -(-(-x)) ),
    TEST_EXPRESSION( (x+y)*x ),
    TEST_EXPRESSION( (x+y)*y ),
    TEST_EXPRESSION( (x+y)*(1.19+z) ),
    TEST_EXPRESSION( ((x+(x+2.13))*y) ),
    TEST_EXPRESSION( (x+y+z*2+(x*z+z*1.5)) ),
    TEST_EXPRESSION( (((((((x-0.28)+y)+x)+x)*x)/1.12)*y) ),
    TEST_EXPRESSION( ((((x*((((y-1.50)+1.82)-x)/PI))/x)*x)+z) ),
    TEST_EXPRESSION( (((((((((x+1.35)+PI)/PI)-y)+z)-z)+y)/x)+0.81) )
  };

  for (int i = 0; i < TABLE_SIZE(tests); i++)
  {
    printf("EXP: %s\n", tests[i].expression);

    if (e0.create(ctx, tests[i].expression, MathPresso::MOPTION_NO_JIT) != MathPresso::MRESULT_OK)
    {
      printf("     Failure: Compilation error (no-jit).\n");
      continue;
    }

    if (e1.create(ctx, tests[i].expression, MathPresso::MOPTION_NONE) != MathPresso::MRESULT_OK)
    {
      printf("     Failure: Compilation error (use-jit).\n");
      continue;
    }

    MathPresso::mreal_t res0 = e0.evaluate(variables);
    MathPresso::mreal_t res1 = e1.evaluate(variables);
    MathPresso::mreal_t expected = tests[i].expected;

    const char* msg0 = fabs((double)res0 - (double)expected) < 0.001 ? "Ok" : "Failure";
    const char* msg1 = fabs((double)res1 - (double)expected) < 0.001 ? "Ok" : "Failure";

    printf(
      "     expected=%f\n"
      "     eval    =%f (%s)\n"
      "     jit     =%f (%s)\n"
      "\n",
      (double)expected,
      (double)res0, msg0,
      (double)res1, msg1);
  }

  return 0;
}
