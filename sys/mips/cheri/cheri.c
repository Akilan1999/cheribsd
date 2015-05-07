/*-
 * Copyright (c) 2011-2015 Robert N. M. Watson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
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

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/sysctl.h>

#include <ddb/ddb.h>
#include <sys/kdb.h>

#include <machine/atomic.h>
#include <machine/cheri.h>
#include <machine/pcb.h>
#include <machine/sysarch.h>

/*
 * Beginnings of a programming interface for explicitly managing capability
 * registers.  Convert back and forth between capability registers and
 * general-purpose registers/memory so that we can program the context,
 * save/restore application contexts, etc.
 *
 * In the future, we'd like the compiler to do this sort of stuff for us
 * based on language-level properties and annotations, but in the mean
 * time...
 *
 * XXXRW: Any manipulation of c0 should include a "memory" clobber for inline
 * assembler, so that the compiler will write back memory contents before the
 * call, and reload them afterwards.
 */

SYSCTL_NODE(_security, OID_AUTO, cheri, CTLFLAG_RD, 0,
    "CHERI settings and statistics");

SYSCTL_NODE(_security_cheri, OID_AUTO, stats, CTLFLAG_RD, 0,
    "CHERI statistics");

/* XXXRW: Should possibly be u_long. */
u_int	security_cheri_syscall_violations;
SYSCTL_UINT(_security_cheri, OID_AUTO, syscall_violations, CTLFLAG_RD,
    &security_cheri_syscall_violations, 0, "Number of system calls blocked");

u_int	security_cheri_sandboxed_signals;
SYSCTL_UINT(_security_cheri, OID_AUTO, sandboxed_signals, CTLFLAG_RD,
    &security_cheri_sandboxed_signals, 0, "Number of signals in sandboxes");

/*
 * A set of sysctls that cause the kernel debugger to enter following a policy
 * violation or signal delivery due to CHERI or while in a sandbox.
 */
u_int	security_cheri_debugger_on_sandbox_signal;
SYSCTL_UINT(_security_cheri, OID_AUTO, debugger_on_sandbox_signal, CTLFLAG_RW,
    &security_cheri_debugger_on_sandbox_signal, 0,
    "Enter KDB when a signal is delivered while in a sandbox");

u_int	security_cheri_debugger_on_sandbox_syscall;
SYSCTL_UINT(_security_cheri, OID_AUTO, debugger_on_sandbox_syscall, CTLFLAG_RW,
    &security_cheri_debugger_on_sandbox_syscall, 0,
    "Enter KDB when a syscall is rejected while in a sandbox");

u_int	security_cheri_debugger_on_sandbox_unwind;
SYSCTL_UINT(_security_cheri, OID_AUTO, debugger_on_sandbox_unwind, CTLFLAG_RW,
    &security_cheri_debugger_on_sandbox_unwind, 0,
    "Enter KDB when a sandbox is auto-unwound due to a signal");

u_int	security_cheri_debugger_on_sigprot;
SYSCTL_UINT(_security_cheri, OID_AUTO, debugger_on_sigprot, CTLFLAG_RW,
    &security_cheri_debugger_on_sigprot, 0,
    "Enter KDB when SIGPROT is delivered to an unsandboxed thread");

static void	cheri_capability_set_user_c0(struct chericap *);
static void	cheri_capability_set_user_stack(struct chericap *);
static void	cheri_capability_set_user_pcc(struct chericap *);

/*
 * For now, all we do is declare what we support, as most initialisation took
 * place in the MIPS machine-dependent assembly.  CHERI doesn't need a lot of
 * actual boot-time initialisation.
 */
static void
cheri_cpu_startup(void)
{

	printf(
#ifdef CPU_CHERI128
	    "CHERI: compiled for 128-bit capabilities\n"
#else
	    "CHERI: compiled for 256-bit capabilities\n"
#endif
	    );
}
SYSINIT(cheri_cpu_startup, SI_SUB_CPU, SI_ORDER_FIRST, cheri_cpu_startup,
    NULL);

/*
 * Given an existing more privileged capability (fromcrn), build a new
 * capability in tocrn with the contents of the passed flattened
 * representation.
 *
 * XXXRW: It's not yet clear how important ordering is here -- try to do the
 * privilege downgrade in a way that will work when doing an "in place"
 * downgrade, with permissions last.
 *
 * XXXRW: How about the sealed bit?
 *
 * XXXRW: In the new world order of CSetBounds, it's not clear that taking
 * explicit base/length/offset arguments is quite the right thing.
 */
void
cheri_capability_set(struct chericap *cp, uint32_t perms, void *otypep,
    void *basep, size_t length, off_t off)
{
#ifdef INVARIANTS
	register_t r;
#endif

	/*
	 * NB: Set fields in this order (offset first), in the absence of a
	 * CSetBounds instruction, such that maximum precision is available
	 * when using compressed capabilities.
	 */
	CHERI_CSETOFFSET(CHERI_CR_CTEMP0, CHERI_CR_KDC, (register_t)basep);
	CHERI_CSETBOUNDS(CHERI_CR_CTEMP0, CHERI_CR_CTEMP0,
	    (register_t)length);
	CHERI_CANDPERM(CHERI_CR_CTEMP0, CHERI_CR_CTEMP0, (register_t)perms);

	/*
	 * NB: With imprecise bounds, offset may be non-zero after setting up
	 * bounds.  We therefore need to add the requested offset to the
	 * actual one before installing, rather than simply setting it.
	 */
	CHERI_CINCOFFSET(CHERI_CR_CTEMP0, CHERI_CR_CTEMP0,
	    (register_t)off - (register_t)basep);
#if 0
	/* XXXRW: For now, don't set type. */
	CHERI_CSETTYPE(CHERI_CR_CTEMP0, CHERI_CR_CTEMP0, (register_t)otypep);
#endif

	/*
	 * NB: With imprecise bounds, we want to assert that the results will
	 * be 'as requested' -- i.e., that the kernel always request bounds
	 * that can be represented precisly.
	 *
	 * XXXRW: Given these assupmtions, we actually don't need to do the
	 * '+= off' above.
	 */
#ifdef INVARIANTS
	CHERI_CGETPERM(r, CHERI_CR_CTEMP0);
	KASSERT(r == (register_t)perms,
	    ("%s: permissions 0x%x rather than 0x%x", __func__,
	    (unsigned int)r, perms));
	CHERI_CGETBASE(r, CHERI_CR_CTEMP0);
	KASSERT(r == (register_t)basep,
	    ("%s: base %p rather than %p", __func__, (void *)r, basep));
	CHERI_CGETLEN(r, CHERI_CR_CTEMP0);
	KASSERT(r == (register_t)length,
	    ("%s: length 0x%x rather than %p", __func__,
	    (unsigned int)r, (void *)length));
	CHERI_CGETOFFSET(r, CHERI_CR_CTEMP0);
	KASSERT(r == (register_t)off,
	    ("%s: offset %p rather than %p", __func__, (void *)r,
	    (void *)off));
#if 0
	CHERI_CGETTYPE(r, CHERI_CR_CTEMP0);
	KASSERT(r == (register_t)otypep,
	    ("%s: otype %p rather than %p", __func__, (void *)r, otypep));
#endif
#endif
	CHERI_CSC(CHERI_CR_CTEMP0, CHERI_CR_KDC, (register_t)cp, 0);
}

static void
cheri_capability_clear(struct chericap *cp)
{

	/*
	 * While we could construct a non-capability and write it out, simply
	 * bzero'ing memory is sufficient to clear the tag bit, and easier to
	 * spell.
	 */
	bzero(cp, sizeof(*cp));
}

/*
 * Functions to store a common set of capability values to in-memory
 * capabilities used in various aspects of user context.
 * contexts.
 */
#ifdef _UNUSED
static void
cheri_capability_set_priv(struct chericap *cp)
{

	cheri_capability_set(cp, CHERI_CAP_PRIV_PERMS, CHERI_CAP_PRIV_OTYPE,
	    CHERI_CAP_PRIV_BASE, CHERI_CAP_PRIV_LENGTH,
	    CHERI_CAP_PRIV_OFFSET);
}
#endif

static void
cheri_capability_set_user_c0(struct chericap *cp)
{

	cheri_capability_set(cp, CHERI_CAP_USER_PERMS, CHERI_CAP_USER_OTYPE,
	    CHERI_CAP_USER_BASE, CHERI_CAP_USER_LENGTH,
	    CHERI_CAP_USER_OFFSET);
}

static void
cheri_capability_set_user_stack(struct chericap *cp)
{

	/*
	 * For now, initialise stack as ambient with identical rights as $c0.
	 * In the future, we will likely want to change this to be local
	 * (non-global).
	 */
	cheri_capability_set(cp, CHERI_CAP_USER_PERMS, CHERI_CAP_USER_OTYPE,
	    CHERI_CAP_USER_BASE, CHERI_CAP_USER_LENGTH,
	    CHERI_CAP_USER_OFFSET);
}

static void
cheri_capability_set_user_idc(struct chericap *cp)
{

	/*
	 * The default invoked data capability is also identical to $c0.
	 */
	cheri_capability_set(cp, CHERI_CAP_USER_PERMS, CHERI_CAP_USER_OTYPE,
	    CHERI_CAP_USER_BASE, CHERI_CAP_USER_LENGTH,
	    CHERI_CAP_USER_OFFSET);
}

static void
cheri_capability_set_user_pcc(struct chericap *cp)
{

	cheri_capability_set(cp, CHERI_CAP_USER_PERMS, CHERI_CAP_USER_OTYPE,
	    CHERI_CAP_USER_BASE, CHERI_CAP_USER_LENGTH,
	    CHERI_CAP_USER_OFFSET);
}

void
cheri_capability_set_null(struct chericap *cp)
{

	/* XXXRW: Should be using CFromPtr(NULL) for this. */
	cheri_capability_clear(cp);
}

/*
 * Because contexts contain tagged capabilities, we can't just use memcpy()
 * on the data structure.  Once the C compiler knows about capabilities, then
 * direct structure assignment should be plausible.  In the mean time, an
 * explicit capability context copy routine is required.
 *
 * XXXRW: Compiler should know how to do copies of tagged capabilities.
 *
 * XXXRW: Compiler should be providing us with the temporary register.
 */
void
cheri_capability_copy(struct chericap *cp_to, struct chericap *cp_from)
{

	cheri_capability_load(CHERI_CR_CTEMP0, cp_from);
	cheri_capability_store(CHERI_CR_CTEMP0, cp_to);
}

void
cheri_context_copy(struct pcb *dst, struct pcb *src)
{

	cheri_memcpy(&dst->pcb_cheriframe, &src->pcb_cheriframe,
	    sizeof(dst->pcb_cheriframe));
}

void
cheri_exec_setregs(struct thread *td)
{
	struct cheri_frame *cfp;
	struct cheri_signal *csigp;

	/*
	 * XXXRW: Experimental CHERI ABI initialises $c0 with full user
	 * privilege, and all other user-accessible capability registers with
	 * no rights at all.  The runtime linker/compiler/application can
	 * propagate around rights as required.
	 */
	cfp = &td->td_pcb->pcb_cheriframe;
	bzero(cfp, sizeof(*cfp));
	cheri_capability_set_user_c0(&cfp->cf_c0);
	cheri_capability_set_user_stack(&cfp->cf_c11);
	cheri_capability_set_user_idc(&cfp->cf_idc);
	cheri_capability_set_user_pcc(&cfp->cf_pcc);

	/*
	 * Also initialise signal-handling state; this can't yet be modified
	 * by userspace, but the principle is that signal handlers should run
	 * with ambient authority unless given up by the userspace runtime
	 * explicitly.
	 */
	csigp = &td->td_pcb->pcb_cherisignal;
	bzero(csigp, sizeof(*csigp));
	cheri_capability_set_user_c0(&csigp->csig_c0);
	cheri_capability_set_user_stack(&csigp->csig_c11);
	cheri_capability_set_user_idc(&csigp->csig_idc);
	cheri_capability_set_user_pcc(&csigp->csig_pcc);
}
