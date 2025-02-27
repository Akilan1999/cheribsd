/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2005 Wojciech A. Koszek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/sbuf.h>
#include <sys/syscallsubr.h>
#include <sys/syslog.h>

#include <compat/freebsd64/freebsd64_proto.h>

int
freebsd64_abort2(struct thread *td, struct freebsd64_abort2_args *uap)
{
	void * __capability uargs[16];
	void *uargsp;
	uint64_t * __capability uargscap;
	uint64_t ptr;
	int i, nargs;

	nargs = uap->nargs;
	if (nargs < 0 || nargs > nitems(uargs))
		nargs = -1;
	uargsp = NULL;
	if (nargs > 0) {
		if (uap->args != NULL) {
			uargscap = __USER_CAP_ARRAY(uap->args, nargs);
			for (i = 0; i < nargs; i++) {
				if (fueword64(uargscap + i, &ptr) != 0) {
					nargs = -1;
					break;
				} else
					uargs[i] = __USER_CAP_UNBOUND(
					    (void *)(uintptr_t)ptr);
			}
			if (nargs > 0)
				uargsp = &uargs;
		} else
			nargs = -1;
	}
	return (kern_abort2(td, __USER_CAP_STR(uap->why), nargs, uargsp));
}
/*
 * CHERI CHANGES START
 * {
 *   "updated": 20230509,
 *   "target_type": "kernel",
 *   "changes": [
 *     "support"
 *   ]
 * }
 * CHERI CHANGES END
 */
