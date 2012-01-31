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

struct Variables
{
  MathPresso::mreal_t x;
  MathPresso::mreal_t y;
  MathPresso::mreal_t z;
};

int main(int argc, char* argv[])
{
  MathPresso::Context ctx;
  MathPresso::Expression e;

  Variables variables;
  variables.x = 0.0;
  variables.y = 0.0;
  variables.z = 0.0;

  ctx.addEnvironment(MathPresso::MENVIRONMENT_ALL);
  ctx.addVariable("x", MATHPRESSO_OFFSET(Variables, x));
  ctx.addVariable("y", MATHPRESSO_OFFSET(Variables, y));
  ctx.addVariable("z", MATHPRESSO_OFFSET(Variables, z));

  fprintf(stdout, "=========================================================\n");
  fprintf(stdout, "MathPresso - Command Line Evaluator\n");
  fprintf(stdout, "(c) 2008-2010, Petr Kobalicek, MIT License.\n");
  fprintf(stdout, "---------------------------------------------------------\n");
  fprintf(stdout, "You can use variables 'x', 'y' and 'z'. Initial values of\n");
  fprintf(stdout, "these variables are 0.0, but using '=' operator the value\n");
  fprintf(stdout, "can be assigned (use for example x = 1).\n");
  fprintf(stdout, "=========================================================\n");

  for (;;)
  {
    char buffer[4096];
    fgets(buffer, 4095, stdin);
    if (buffer[0] == 0) break;

    MathPresso::mresult_t result = e.create(ctx, buffer);
    if (result == MathPresso::MRESULT_NO_EXPRESSION) break;

    if (result != MathPresso::MRESULT_OK)
    {
      fprintf(stderr, "Error compiling expression:\n%s\n", buffer);
      break;
    }
    else
    {
      fprintf(stdout, "%f\n", e.evaluate(&variables));
    }
  }

  return 0;
}
