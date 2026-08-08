/*
 * dwnx
 *
 * Copyright (c) 2026 dwnx contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef DWNX_FMT_H
#define DWNX_FMT_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

typedef struct dwnx_fmt_hex {
  uint64_t n;
} dwnx_fmt_hex;

static inline dwnx_fmt_hex dwnx_fmt_hex_init(uint64_t n) {
  return (dwnx_fmt_hex){
    .n = n,
  };
}

static inline dwnx_fmt_hex dwnx_fmt_hex_signed_init(int64_t n) {
  return dwnx_fmt_hex_init((uint64_t)n);
}

static inline dwnx_fmt_hex dwnx_fmt_hex_long_int_init(long int n) {
  return dwnx_fmt_hex_init((unsigned long int)n);
}

static inline dwnx_fmt_hex dwnx_fmt_hex_int_init(int n) {
  return dwnx_fmt_hex_init((unsigned int)n);
}

static inline dwnx_fmt_hex dwnx_fmt_hex_short_int_init(short int n) {
  return dwnx_fmt_hex_init((unsigned short int)n);
}

static inline dwnx_fmt_hex dwnx_fmt_hex_signed_char_init(signed char n) {
  return dwnx_fmt_hex_init((unsigned char)n);
}

static inline dwnx_fmt_hex dwnx_fmt_hex_char_init(char n) {
  return dwnx_fmt_hex_init((unsigned char)n);
}

/* hex formats integral |T| in unsigned hexadecimal notation.  It
   drops the leading zeros. */
#define hex(T)                                                                 \
  _Generic((T),                                                                \
    long long int: dwnx_fmt_hex_signed_init,                                   \
    long int: dwnx_fmt_hex_long_int_init,                                      \
    int: dwnx_fmt_hex_int_init,                                                \
    short int: dwnx_fmt_hex_short_int_init,                                    \
    signed char: dwnx_fmt_hex_signed_char_init,                                \
    char: dwnx_fmt_hex_char_init,                                              \
    unsigned long long int: dwnx_fmt_hex_init,                                 \
    unsigned long int: dwnx_fmt_hex_init,                                      \
    unsigned int: dwnx_fmt_hex_init,                                           \
    unsigned short int: dwnx_fmt_hex_init,                                     \
    unsigned char: dwnx_fmt_hex_init)((T))

typedef struct dwnx_fmt_hexw {
  uint64_t n;
  size_t width;
} dwnx_fmt_hexw;

static inline dwnx_fmt_hexw dwnx_fmt_hexw_init(uint64_t n, size_t width) {
  return (dwnx_fmt_hexw){
    .n = n,
    .width = width,
  };
}

static inline dwnx_fmt_hexw dwnx_fmt_hexw_signed_init(int64_t n, size_t width) {
  return dwnx_fmt_hexw_init((uint64_t)n, width);
}

static inline dwnx_fmt_hexw dwnx_fmt_hexw_long_int_init(long int n,
                                                        size_t width) {
  return dwnx_fmt_hexw_init((unsigned long int)n, width);
}

static inline dwnx_fmt_hexw dwnx_fmt_hexw_int_init(int n, size_t width) {
  return dwnx_fmt_hexw_init((unsigned int)n, width);
}

static inline dwnx_fmt_hexw dwnx_fmt_hexw_short_int_init(short int n,
                                                         size_t width) {
  return dwnx_fmt_hexw_init((unsigned short int)n, width);
}

static inline dwnx_fmt_hexw dwnx_fmt_hexw_signed_char_init(signed char n,
                                                           size_t width) {
  return dwnx_fmt_hexw_init((unsigned char)n, width);
}

static inline dwnx_fmt_hexw dwnx_fmt_hexw_char_init(char n, size_t width) {
  return dwnx_fmt_hexw_init((unsigned char)n, width);
}

/* hex formats integral |T| in unsigned hexadecimal notation.  If the
   produced value has fewer characters than |WIDTH|, it will be padded
   with 0 on the left. */
#define hexw(T, WIDTH)                                                         \
  _Generic((T),                                                                \
    long long int: dwnx_fmt_hexw_signed_init,                                  \
    long int: dwnx_fmt_hexw_long_int_init,                                     \
    int: dwnx_fmt_hexw_int_init,                                               \
    short int: dwnx_fmt_hexw_short_int_init,                                   \
    signed char: dwnx_fmt_hexw_signed_char_init,                               \
    char: dwnx_fmt_hexw_char_init,                                             \
    unsigned long long int: dwnx_fmt_hexw_init,                                \
    unsigned long int: dwnx_fmt_hexw_init,                                     \
    unsigned int: dwnx_fmt_hexw_init,                                          \
    unsigned short int: dwnx_fmt_hexw_init,                                    \
    unsigned char: dwnx_fmt_hexw_init)((T), (WIDTH))

typedef struct dwnx_fmt_uint64w {
  uint64_t n;
  size_t width;
} dwnx_fmt_uint64w;

/* uintw formats integral |n|.  If the produced value has fewer
   characters than |width|, it will be padded with 0 on the left. */
static inline dwnx_fmt_uint64w uintw(uint64_t n, size_t width) {
  return (dwnx_fmt_uint64w){
    .n = n,
    .width = width,
  };
}

typedef struct dwnx_fmt_bhex {
  const uint8_t *data;
  size_t len;
} dwnx_fmt_bhex;

/* bhex formats the binary data [|data|, |data| + |len|) in
   hexadecimal notation. */
static inline dwnx_fmt_bhex bhex(const uint8_t *data, size_t len) {
  return (dwnx_fmt_bhex){
    .data = data,
    .len = len,
  };
}

/* lbhex formats the binary data [|B|, |B| + sizeof(|B|)) in
   hexadecimal notation.  To make it work, |B| must be an array that
   is not decayed to the pointer. */
#define lbhex(B) bhex((B), sizeof(B))

typedef struct dwnx_fmt_printable_ascii {
  const uint8_t *data;
  size_t len;
} dwnx_fmt_printable_ascii;

/* ascii formats the binary data [|data|, |data| + |len|) in such a
   way that the printable ASCII characters are copied as is, and the
   other characters are converted to '.'. */
static inline dwnx_fmt_printable_ascii ascii(const uint8_t *data, size_t len) {
  return (dwnx_fmt_printable_ascii){
    .data = data,
    .len = len,
  };
}

#define dwnx_fmt_stringify(M) #M

/* stringify converts macro |M| to string literal.*/
#define stringify(M) dwnx_fmt_stringify(M)

char *dwnx_fmt_write_int64(char *dest, int64_t n);
char *dwnx_fmt_write_uint64(char *dest, uint64_t n);
char *dwnx_fmt_write_char(char *dest, char c);
char *dwnx_fmt_write_str(char *dest, const char *s);
char *dwnx_fmt_write_uint64w(char *dest, dwnx_fmt_uint64w f);
char *dwnx_fmt_write_hex(char *dest, dwnx_fmt_hex f);
char *dwnx_fmt_write_hexw(char *dest, dwnx_fmt_hexw f);
char *dwnx_fmt_write_bhex(char *dest, dwnx_fmt_bhex f);
char *dwnx_fmt_write_printable_ascii(char *dest,
                                     const dwnx_fmt_printable_ascii f);

#define DWNX_FMT_WRITE_TYPE(DEST, T)                                           \
  _Generic((T),                                                                \
    long long int: dwnx_fmt_write_int64,                                       \
    long int: dwnx_fmt_write_int64,                                            \
    int: dwnx_fmt_write_int64,                                                 \
    short int: dwnx_fmt_write_int64,                                           \
    signed char: dwnx_fmt_write_int64,                                         \
    char: dwnx_fmt_write_char,                                                 \
    unsigned long long int: dwnx_fmt_write_uint64,                             \
    unsigned long int: dwnx_fmt_write_uint64,                                  \
    unsigned int: dwnx_fmt_write_uint64,                                       \
    unsigned short int: dwnx_fmt_write_uint64,                                 \
    unsigned char: dwnx_fmt_write_uint64,                                      \
    char *: dwnx_fmt_write_str,                                                \
    const char *: dwnx_fmt_write_str,                                          \
    dwnx_fmt_uint64w: dwnx_fmt_write_uint64w,                                  \
    dwnx_fmt_hex: dwnx_fmt_write_hex,                                          \
    dwnx_fmt_hexw: dwnx_fmt_write_hexw,                                        \
    dwnx_fmt_bhex: dwnx_fmt_write_bhex,                                        \
    dwnx_fmt_printable_ascii: dwnx_fmt_write_printable_ascii)((DEST), (T))

/* dwnx_fmt_format formats arguments and writes them into the buffer
   pointed by |BUF|.  It also writes the terminal NUL byte.  The
   function assumes that the buffer is the large enough.  It assigns
   the number of bytes written, excluding the terminal NUL, to
   |*PNWRITE|. */

/* Generated by fmtgen.py */
#define DWNX_FMT_SELECT_WRITE_PACK(                                            \
  _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17,  \
  _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,   \
  _33, _34, _35, _36, _37, _38, _39, _40, PACK, ...)                           \
  PACK

#define dwnx_fmt_format(BUF, PNWRITE, ...)                                     \
  do {                                                                         \
    char *fmt_destp = (char *)(BUF);                                           \
    DWNX_FMT_SELECT_WRITE_PACK(                                                \
      __VA_ARGS__, DWNX_FMT_WRITE_PACK_40, DWNX_FMT_WRITE_PACK_39,             \
      DWNX_FMT_WRITE_PACK_38, DWNX_FMT_WRITE_PACK_37, DWNX_FMT_WRITE_PACK_36,  \
      DWNX_FMT_WRITE_PACK_35, DWNX_FMT_WRITE_PACK_34, DWNX_FMT_WRITE_PACK_33,  \
      DWNX_FMT_WRITE_PACK_32, DWNX_FMT_WRITE_PACK_31, DWNX_FMT_WRITE_PACK_30,  \
      DWNX_FMT_WRITE_PACK_29, DWNX_FMT_WRITE_PACK_28, DWNX_FMT_WRITE_PACK_27,  \
      DWNX_FMT_WRITE_PACK_26, DWNX_FMT_WRITE_PACK_25, DWNX_FMT_WRITE_PACK_24,  \
      DWNX_FMT_WRITE_PACK_23, DWNX_FMT_WRITE_PACK_22, DWNX_FMT_WRITE_PACK_21,  \
      DWNX_FMT_WRITE_PACK_20, DWNX_FMT_WRITE_PACK_19, DWNX_FMT_WRITE_PACK_18,  \
      DWNX_FMT_WRITE_PACK_17, DWNX_FMT_WRITE_PACK_16, DWNX_FMT_WRITE_PACK_15,  \
      DWNX_FMT_WRITE_PACK_14, DWNX_FMT_WRITE_PACK_13, DWNX_FMT_WRITE_PACK_12,  \
      DWNX_FMT_WRITE_PACK_11, DWNX_FMT_WRITE_PACK_10, DWNX_FMT_WRITE_PACK_9,   \
      DWNX_FMT_WRITE_PACK_8, DWNX_FMT_WRITE_PACK_7, DWNX_FMT_WRITE_PACK_6,     \
      DWNX_FMT_WRITE_PACK_5, DWNX_FMT_WRITE_PACK_4, DWNX_FMT_WRITE_PACK_3,     \
      DWNX_FMT_WRITE_PACK_2, DWNX_FMT_WRITE_PACK_1)(fmt_destp, __VA_ARGS__);   \
    *fmt_destp = '\0';                                                         \
    *(PNWRITE) = (size_t)(fmt_destp - (char *)(BUF));                          \
  } while (0)

#define DWNX_FMT_WRITE_PACK_1(DEST, _1)                                        \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1))
#define DWNX_FMT_WRITE_PACK_2(DEST, _1, _2)                                    \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2))
#define DWNX_FMT_WRITE_PACK_3(DEST, _1, _2, _3)                                \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3))
#define DWNX_FMT_WRITE_PACK_4(DEST, _1, _2, _3, _4)                            \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4))
#define DWNX_FMT_WRITE_PACK_5(DEST, _1, _2, _3, _4, _5)                        \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5))
#define DWNX_FMT_WRITE_PACK_6(DEST, _1, _2, _3, _4, _5, _6)                    \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6))
#define DWNX_FMT_WRITE_PACK_7(DEST, _1, _2, _3, _4, _5, _6, _7)                \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7))
#define DWNX_FMT_WRITE_PACK_8(DEST, _1, _2, _3, _4, _5, _6, _7, _8)            \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8))
#define DWNX_FMT_WRITE_PACK_9(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9)        \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9))
#define DWNX_FMT_WRITE_PACK_10(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10)  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10))
#define DWNX_FMT_WRITE_PACK_11(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11)                                            \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11))
#define DWNX_FMT_WRITE_PACK_12(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12)                                       \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12))
#define DWNX_FMT_WRITE_PACK_13(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13)                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13))
#define DWNX_FMT_WRITE_PACK_14(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14)                             \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14))
#define DWNX_FMT_WRITE_PACK_15(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15)                        \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15))
#define DWNX_FMT_WRITE_PACK_16(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16)                   \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16))
#define DWNX_FMT_WRITE_PACK_17(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17)              \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17))
#define DWNX_FMT_WRITE_PACK_18(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18)         \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18))
#define DWNX_FMT_WRITE_PACK_19(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19)    \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19))
#define DWNX_FMT_WRITE_PACK_20(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20)                                            \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20))
#define DWNX_FMT_WRITE_PACK_21(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21)                                       \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21))
#define DWNX_FMT_WRITE_PACK_22(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22)                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22))
#define DWNX_FMT_WRITE_PACK_23(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23)                             \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23))
#define DWNX_FMT_WRITE_PACK_24(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24)                        \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24))
#define DWNX_FMT_WRITE_PACK_25(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24, _25)                   \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25))
#define DWNX_FMT_WRITE_PACK_26(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24, _25, _26)              \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26))
#define DWNX_FMT_WRITE_PACK_27(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24, _25, _26, _27)         \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27))
#define DWNX_FMT_WRITE_PACK_28(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24, _25, _26, _27, _28)    \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28))
#define DWNX_FMT_WRITE_PACK_29(                                                \
  DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
  _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29)             \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29))
#define DWNX_FMT_WRITE_PACK_30(                                                \
  DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
  _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30)        \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30))
#define DWNX_FMT_WRITE_PACK_31(                                                \
  DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
  _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31)   \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_31))
#define DWNX_FMT_WRITE_PACK_32(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24, _25, _26, _27, _28,    \
                               _29, _30, _31, _32)                             \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_31));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_32))
#define DWNX_FMT_WRITE_PACK_33(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24, _25, _26, _27, _28,    \
                               _29, _30, _31, _32, _33)                        \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_31));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_32));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_33))
#define DWNX_FMT_WRITE_PACK_34(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24, _25, _26, _27, _28,    \
                               _29, _30, _31, _32, _33, _34)                   \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_31));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_32));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_33));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_34))
#define DWNX_FMT_WRITE_PACK_35(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24, _25, _26, _27, _28,    \
                               _29, _30, _31, _32, _33, _34, _35)              \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_31));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_32));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_33));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_34));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_35))
#define DWNX_FMT_WRITE_PACK_36(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24, _25, _26, _27, _28,    \
                               _29, _30, _31, _32, _33, _34, _35, _36)         \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_31));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_32));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_33));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_34));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_35));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_36))
#define DWNX_FMT_WRITE_PACK_37(DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,  \
                               _11, _12, _13, _14, _15, _16, _17, _18, _19,    \
                               _20, _21, _22, _23, _24, _25, _26, _27, _28,    \
                               _29, _30, _31, _32, _33, _34, _35, _36, _37)    \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_31));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_32));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_33));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_34));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_35));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_36));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_37))
#define DWNX_FMT_WRITE_PACK_38(                                                \
  DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
  _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31,   \
  _32, _33, _34, _35, _36, _37, _38)                                           \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_31));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_32));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_33));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_34));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_35));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_36));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_37));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_38))
#define DWNX_FMT_WRITE_PACK_39(                                                \
  DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
  _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31,   \
  _32, _33, _34, _35, _36, _37, _38, _39)                                      \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_31));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_32));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_33));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_34));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_35));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_36));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_37));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_38));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_39))
#define DWNX_FMT_WRITE_PACK_40(                                                \
  DEST, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
  _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31,   \
  _32, _33, _34, _35, _36, _37, _38, _39, _40)                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_1));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_2));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_3));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_4));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_5));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_6));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_7));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_8));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_9));                                  \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_10));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_11));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_12));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_13));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_14));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_15));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_16));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_17));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_18));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_19));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_20));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_21));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_22));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_23));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_24));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_25));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_26));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_27));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_28));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_29));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_30));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_31));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_32));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_33));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_34));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_35));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_36));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_37));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_38));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_39));                                 \
  (DEST) = DWNX_FMT_WRITE_TYPE((DEST), (_40))

#endif /* !defined(DWNX_FMT_H) */
