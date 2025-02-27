/*-
 * Copyright (c) 2003 Alan L. Cox <alc@cs.rice.edu>
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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_page.h>
#include <vm/vm_phys.h>
#include <vm/vm_dumpset.h>
#include <vm/uma.h>
#include <vm/uma_int.h>
#include <machine/machdep.h>

#include <cheri/cheric.h>

void *
uma_small_alloc(uma_zone_t zone, vm_size_t bytes, int domain, u_int8_t *flags,
    int wait)
{
	vm_page_t m;
	vm_paddr_t pa;

	*flags = UMA_SLAB_PRIV;
	m = vm_page_alloc_noobj_domain(domain, malloc2vm_flags(wait) |
	    VM_ALLOC_WIRED);
	if (m == NULL)
		return (NULL);
	pa = m->phys_addr;
	if ((wait & M_NODUMP) == 0)
		dump_add_page(pa);
	KASSERT(bytes == PAGE_SIZE, ("%s: invalid allocation size %zu",
	    __func__, bytes));
	return ((void *)PHYS_TO_DMAP_PAGE(pa));
}

void
uma_small_free(void *mem, vm_size_t size, u_int8_t flags)
{
	vm_page_t m;
	vm_paddr_t pa;

#ifdef __CHERI_PURE_CAPABILITY__
	KASSERT(!cheri_getsealed(mem),
	    ("uma_small_free: Unexpected sealed capability %#p", mem));
	KASSERT(cheri_gettag(mem),
	    ("uma_small_free: Attempt to free invalid capability %#p", mem));
#endif
	pa = DMAP_TO_PHYS((vm_offset_t)mem);
	dump_drop_page(pa);
	m = PHYS_TO_VM_PAGE(pa);
	vm_page_unwire_noq(m);
	vm_page_free(m);
}
