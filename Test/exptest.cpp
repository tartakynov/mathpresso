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

// Some test values
#define INITVARS (x = 5.1f, y = 6.7f, z = 9.9f, t = 0)

#define TABLE_SIZE(table) \
  (sizeof(table) / sizeof(table[0]))

#define TEST_EXPRESSION(expression) \
  { #expression, (INITVARS, expression) }

#define ADDCONST(c) addConstant(#c, (c))


int main(int argc, char* argv[])
{
  MathPresso::mreal_t x = 5.1f;
  MathPresso::mreal_t y = 6.7f;
  MathPresso::mreal_t z = 9.9f;
  MathPresso::mreal_t t = 0;

  const double PI = 3.14159265358979323846;

  MathPresso::mreal_t cx = cos(PI/3);
  MathPresso::mreal_t cy = sin(PI/3);
  MathPresso::mreal_t ox = 0.5;
  MathPresso::mreal_t oy = 1.75;

  MathPresso::Context ctx;
  MathPresso::Expression e0, e1, e2;

  ctx.addEnvironment(MathPresso::MENVIRONMENT_ALL);
  ctx.addVariable("x", 0 * sizeof(MathPresso::mreal_t));
  ctx.addVariable("y", 1 * sizeof(MathPresso::mreal_t));
  ctx.addVariable("z", 2 * sizeof(MathPresso::mreal_t));
  ctx.addVariable("t", 3 * sizeof(MathPresso::mreal_t));

  ctx.ADDCONST(cx);
  ctx.ADDCONST(cy);
  ctx.ADDCONST(ox);
  ctx.ADDCONST(oy);

  MathPresso::mreal_t variables[] = { x, y, z, t };

  TestExpression tests[] = {
    TEST_EXPRESSION( (x+y) ),
    TEST_EXPRESSION( -(x-y) ),
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
    TEST_EXPRESSION( (((((((((x+1.35)+PI)/PI)-y)+z)-z)+y)/x)+0.81) ),
    TEST_EXPRESSION( 1+(x+2)+3 ),
    TEST_EXPRESSION( 1+(x+y)+z ),
    TEST_EXPRESSION( x=2*3+1 ),
    TEST_EXPRESSION( (x+y)*z ),
    TEST_EXPRESSION( (x=y)*x ),
    TEST_EXPRESSION( x=y=z ),
    TEST_EXPRESSION( x=(y+(z=5)) ),
    // functions
    TEST_EXPRESSION( log(exp(x * PI/2)) ),
    TEST_EXPRESSION( hypot(x, y) ),
    TEST_EXPRESSION( cos(PI/4)*x - sin(PI/4)*y ),
    TEST_EXPRESSION( sqrt(x*x + y*y + z*z) ),
    // operator ^
    { "sqrt(x^2 + y^2 + z^2)", (INITVARS, sqrt(x*x + y*y + z*z)) },
    { "x^-2 + y^-3", (INITVARS, 1/(x*x) + pow(y, -3)) },
    // semicolon is comma in C++
    { "z=x;x=3*x+1*y;y=1*x-3*z", (INITVARS, z=x,x=3*x+1*y,y=1*x-3*z) },
    { "t = cx*x - cy*y + ox; y = cy*x + cx*y + oy; x = t", (INITVARS, t=cx*x - cy*y + ox, y = cy*x + cx*y + oy, x = t) },
    { "x=cx;y=cy;t=z;z=t;", (INITVARS, x=cx,y=cy,t=z,z=t) },
    // assignment
    { "z = 1*z - 0*z + 1", (INITVARS, z = 1*z - 0*z + 1) },
    { "t = cx*x - cy*y + ox", (INITVARS, t=cx*x - cy*y + ox) },
    { "t = (cx*x - cy*y + ox)", (INITVARS, t=cx*x - cy*y + ox) },
    // unary operators
    TEST_EXPRESSION( 2 * + x ),
    TEST_EXPRESSION( 2 * + + y ),
    TEST_EXPRESSION( -x * -z ),
    TEST_EXPRESSION( 2 * - -t ),
    TEST_EXPRESSION( -2 * - - -3.5 ),
    // optimization tests
    TEST_EXPRESSION( x = 2 * - - - + + - 2 + 0*y + z/1 ),
    { "1*x - 0*y + z^1 - t/-1 + 0", (INITVARS, 1*x - 0*y + pow(z, 1) - t/-1 + 0) },
    { "sin(x*1^t) - cos(0*y + PI) + z^(-4/(-2-2))", (INITVARS, sin(x*pow(1,t)) - cos(0*y + PI) + pow(z, -4/(-2-2)) ) }
  };

  int numok0 = 0,
      numok1 = 0,
      numok2 = 0;

  int n = TABLE_SIZE(tests);
  for (int i = 0; i < n; ++i)
  {
    printf("EXP #%d: %s\n", i+1, tests[i].expression);

    if (e0.create(ctx, tests[i].expression, MathPresso::MOPTION_NO_JIT | MathPresso::MOPTION_NO_OPTIMIZE
        | MathPresso::MOPTION_VERBOSE) != MathPresso::MRESULT_OK)
    {
      printf("     Failure: Compilation error (no JIT).\n");
	  getchar();
      continue;
    }
    printf("\nUnoptimized RPN : %s\n", e0.getRPN().c_str());

    if (e1.create(ctx, tests[i].expression, MathPresso::MOPTION_NO_OPTIMIZE) != MathPresso::MRESULT_OK)
    {
      printf("     Failure: Compilation error (JIT).\n");
	  getchar();
      continue;
    }

    if (e2.create(ctx, tests[i].expression, MathPresso::MOPTION_VERBOSE) != MathPresso::MRESULT_OK)
    {
      printf("     Failure: Compilation error (optimized JIT).\n");
	  getchar();
      continue;
    }
    printf("\nOptimized RPN   : %s\n", e2.getRPN().c_str());

    INITVARS;
    printf("\nBefore:    x= %f  y= %f  z= %f  t= %f\n", x, y, z, t);
    variables[0] = x; variables[1] = y; variables[2] = z; variables[3] = t;
    MathPresso::mreal_t res0 = e0.evaluate(variables);

    INITVARS;
    variables[0] = x; variables[1] = y; variables[2] = z; variables[3] = t;
    MathPresso::mreal_t res1 = e1.evaluate(variables);

    INITVARS;
    variables[0] = x; variables[1] = y; variables[2] = z; variables[3] = t;
    MathPresso::mreal_t res2 = e2.evaluate(variables);

    printf("\nAfter:     x= %f  y= %f  z= %f  t= %f\n", variables[0], variables[1], variables[2], variables[3]);

    MathPresso::mreal_t expected = tests[i].expected;

    bool ok0 = fabs((double)res0 - (double)expected) < 0.0000001;
    bool ok1 = fabs((double)res1 - (double)expected) < 0.0000001;
    bool ok2 = fabs((double)res2 - (double)expected) < 0.0000001;
    if (ok0) numok0++;
    if (ok1) numok1++;
    if (ok2) numok2++;

    printf("\n"
      "     expected      = %f\n"
      "     eval          = %f (%s)\n"
      "     JIT           = %f (%s)\n"
      "     optimized JIT = %f (%s)\n"
      "\n",
      (double)expected,
      (double)res0, ok0 ? "Ok" : "Failure",
      (double)res1, ok1 ? "Ok" : "Failure",
      (double)res2, ok2 ? "Ok" : "Failure");
      //getchar();
  }

  printf("eval:    %d of %d ok\n"
         "jit:     %d of %d ok\n"
         "op_jit:  %d of %d ok\n", numok0, n, numok1, n, numok2, n);
  //getchar();

  MathPresso::mresult_t result;
  do {
    char buffer[4096];
    fgets(buffer, 4095, stdin);
    if (buffer[0] == 0) break;

    MathPresso::Expression e;
    result = e.create(ctx, buffer, MathPresso::MOPTION_VERBOSE);
    if (result == MathPresso::MRESULT_NO_EXPRESSION)
      printf("No expression\n");

    if (result != MathPresso::MRESULT_OK)
    {
      //printf("%s\n", buffer);
      int errorPos = e.getErrorPos();
      for (int j = 0; j < errorPos; ++j) printf(" ");
      printf("^\n");
      printf("Error: %s at pos=%d\n", e.getErrorMessage(), errorPos);
    }
    else
    {
      printf("\nOptimized RPN   : %s\n", e.getRPN().c_str());
      printf("\n%s\n", e.getJitLog().c_str());
      printf("Result = %9.20g\n", e.evaluate(&variables));
    }
  } while(result != MathPresso::MRESULT_NO_EXPRESSION);

  return 0;
}
