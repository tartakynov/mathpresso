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
#include "MathPresso_Util_p.h"

namespace MathPresso {

// ============================================================================
// [MathPresso::Assert]
// ============================================================================

void mpAssertionFailure(const char* expression, int line)
{
  fprintf(stderr, "*** ASSERTION FAILURE: MathPresso.cpp (line %d)\n", line);
  fprintf(stderr, "*** REASON: %s\n", expression);
  exit(0);
}

// ============================================================================
// [MathPresso::mpConvertToFloat]
// ============================================================================

MATHPRESSO_HIDDEN mreal_t mpConvertToFloat(const char* str, size_t length, bool* ok)
{
  double result = 0.0;
  const char* end = str + length;

  // Integer portion.
  while (str != end && mpIsDigit(*str))
  {
    result *= 10;
    result += (double)(*str - '0');
    str++;
  }

  // Fraction.
  if (str != end && *str == '.')
  {
    str++;
    double scale = 0.1;
    while (str != end && mpIsDigit(*str))
    {
       result += (double)(*str - '0') * scale;
       scale *= 0.1;
       str++;
    }
  }

  *ok = (str == end);
  return (mreal_t)result;
}

// ============================================================================
// [MathPresso::StringBuilder]
// ============================================================================

StringBuilder::StringBuilder() :
  _capacity(0),
  _length(0),
  _data(NULL),
  _outOfMemory(false)
{
}

StringBuilder::~StringBuilder()
{
  if (_data) free(_data);
}

StringBuilder& StringBuilder::clear()
{
  _length = 0;
  _outOfMemory = false;

  return *this;
}

StringBuilder& StringBuilder::appendString(const char* str, size_t len)
{
  if (len == MP_INVALID_INDEX) len = strlen(str);
  if (len == 0 || !_grow(len)) return *this;

  memcpy(_data + _length, str, len);
  _length += len;

  return *this;
}

StringBuilder& StringBuilder::appendEscaped(const char* str, size_t len)
{
  if (len == MP_INVALID_INDEX) len = strlen(str);
  if (len == 0 || !_grow(len * 2)) return *this;

  char* dst = _data + _length;

  for (size_t i = 0; i < len; i++)
  {
    char c = str[i];
    switch (c)
    {
      case '\"':
      case '\n':
      case '\r':
        *dst++ = '\\';
      default:
        *dst++ = c;
        break;
    }
  }

  _length = (size_t)(dst - _data);
  return *this;
}

StringBuilder& StringBuilder::appendFormat(const char* fmt, ...)
{
  char buf[1024];
  size_t len;

  va_list ap;
  va_start(ap, fmt);
  len = vsnprintf(buf, 1024, fmt, ap);
  va_end(ap);

  return appendString(buf, len);
}

char* StringBuilder::toString() const
{
  if (_outOfMemory || _length == 0) return NULL;

  char* result = reinterpret_cast<char*>(malloc(_length + 1));
  if (result == NULL) return NULL;

  memcpy(result, _data, _length);
  result[_length] = 0;
  return result;
}

bool StringBuilder::_grow(size_t s)
{
  return _reserve(_length + s);
}

bool StringBuilder::_reserve(size_t s)
{
  if (_capacity < s)
  {
    size_t newCapacity = _capacity;
    if (newCapacity == 0) newCapacity = 1000;

    do {
      newCapacity *= 2;
    } while (newCapacity < s);

    char *newData = reinterpret_cast<char*>(realloc(_data, newCapacity));
    if (newData == NULL)
    {
      _outOfMemory = true;
      return false;
    }

    _data = newData;
    _capacity = newCapacity;
  }

  return true;
}

// ============================================================================
// [MathPresso::Hash<T>]
// ============================================================================

static const int mpPrimeTable[] = {
  23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
  49157, 98317, 196613, 393241, 786433, 1572869, 3145739
};

int mpGetPrime(int x)
{
  int prime = 0;

  for (int i = 0; i < sizeof(mpPrimeTable) / sizeof(mpPrimeTable[0]); i++)
  {
    prime = mpPrimeTable[0];
    if (prime > x) break;
  }
  return prime;
}

unsigned int mpGetHash(const char* _key, size_t klen)
{
  const unsigned char* key = reinterpret_cast<const unsigned char*>(_key);
  unsigned int hash = 0x12345678;
  if (!klen) return 0;

  do {
    unsigned int c = *key++;
    hash ^= ((hash << 5) + (hash >> 2) + c);
  } while (--klen);

  return hash;
}

} // MathPresso namespace
