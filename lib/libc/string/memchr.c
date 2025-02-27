/*-
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2005-2014 Rich Felker, et al.
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
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <sys/cdefs.h>
#include <sys/param.h>

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheric.h>
#endif

#include <limits.h>
#include <stdint.h>
#include <string.h>

#define SS (sizeof(size_t))
#define ONES ((size_t)-1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x)-ONES) & ~(x)&HIGHS)

void *
memchr(const void *src, int c, size_t n)
{
	const unsigned char *s = src;
	c = (unsigned char)c;
#if defined(__GNUC__)
#ifdef __CHERI_PURE_CAPABILITY__
	/*
	 * Make sure the word-wise reads don't walk off the end
	 * of caps with weakly-aligned ends.
	 */
	size_t space = cheri_bytes_remaining(src);
	size_t excess = n - MIN(n, space);
	n = MIN(n, space);
#endif
	for (; !__builtin_is_aligned(s, SS) && n && *s != c; s++, n--)
		;
	if (n && *s != c) {
		typedef size_t __attribute__((__may_alias__)) word;
		const word *w;
		size_t k = ONES * c;
		for (w = (const void *)s; n >= SS && !HASZERO(*w ^ k);
		     w++, n -= SS)
			;
		s = (const void *)w;
	}
#ifdef __CHERI_PURE_CAPABILITY__
	/* Restore excess length so the byte-wise reads trap if appropriate. */
	n += excess;
#endif
#endif
	for (; n && *s != c; s++, n--)
		;
	return n ? (void *)s : 0;
}
