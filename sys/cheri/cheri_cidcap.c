/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 SRI International
 *
 * This software was developed by SRI International, the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology), and Capabilities Limited under Defense Advanced Research
 * Projects Agency (DARPA) Contract No. HR001122S0003 ("MTSS").
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
#include <sys/syscallsubr.h>
#include <sys/sysctl.h>
#include <sys/sysproto.h>
#include <sys/systm.h>

#if __has_feature(capabilities)
#include <cheri/cheri.h>
#include <cheri/cheric.h>
#include <cheri/cherireg.h>
#endif

#include <vm/vm_extern.h>

#if __has_feature(capabilities) && defined(CHERI_PERM_COMPARTMENT_ID)

/* Set to -1 to prevent it from being zeroed with the rest of BSS */
uintcap_t userspace_root_cidcap = (uintcap_t)-1;
SYSCTL_CAPABILITY(_security_cheri, OID_AUTO, cidcap, CTLFLAG_RD | CTLFLAG_PTROUT,
    &userspace_root_cidcap, 0, "CHERI compartment ID root capability");

int
kern_cheri_cidcap_alloc(struct thread *td, uintcap_t * __capability cidp)
{
	uint64_t cid;
	uintcap_t cidcap;

	cid = vmspace_cid_alloc(td->td_proc->p_vmspace);
	cidcap = (uintcap_t)cheri_setbounds(
	    cheri_setaddress(userspace_root_cidcap, cid), 1);

	KASSERT(cheri_gettag(cidcap), ("untagged cidcap allocated"));

	return (copyoutcap(&cidcap, cidp, sizeof(cidcap)));
}

int
sys_cheri_cidcap_alloc(struct thread *td, struct cheri_cidcap_alloc_args *uap)
{
	return (kern_cheri_cidcap_alloc(td, uap->cidp));
}

#else /* !(__has_feature(capabilities) && defined(CHERI_PERM_COMPARTMENT_ID)) */

int
sys_cheri_cidcap_alloc(struct thread *td, struct cheri_cidcap_alloc_args *uap)
{
	return (ENOSYS);
}

#endif /* !(__has_feature(capabilities) && defined(CHERI_PERM_COMPARTMENT_ID)) */
