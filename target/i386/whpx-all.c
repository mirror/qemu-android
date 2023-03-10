/*
 * QEMU Windows Hypervisor Platform accelerator (WHPX)
 *
 * Copyright Microsoft Corp. 2017
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/address-spaces.h"
#include "exec/exec-all.h"
#include "exec/ioport.h"
#include "exec/ram_addr.h"
#include "exec/memory-remap.h"
#include "qemu-common.h"
#ifndef _MSC_VER
#include "strings.h"
#endif
#include "sysemu/accel.h"
#include "sysemu/whpx.h"
#include "sysemu/sysemu.h"
#include "sysemu/cpus.h"
#include "qemu/abort.h"
#include "qemu/main-loop.h"
#include "hw/boards.h"
#include "qemu/error-report.h"
#include "qemu/queue.h"
#include "qapi/error.h"
#include "migration/migration.h"
#include "whp-dispatch.h"

#include <sdkddkver.h>
#if !defined(NTDDI_WIN10_CO)
#include "./whpx-headers/WinHvPlatformDefs.h"
#endif
#include <WinHvPlatform.h>
#include <WinHvEmulation.h>

#define WHPX_CPUID_SIGNATURE 0x40000000

struct whpx_state {
    uint64_t mem_quota;
    WHV_PARTITION_HANDLE partition;
};

static const WHV_REGISTER_NAME whpx_register_names[] = {

    /* X64 General purpose registers */
    WHvX64RegisterRax,
    WHvX64RegisterRcx,
    WHvX64RegisterRdx,
    WHvX64RegisterRbx,
    WHvX64RegisterRsp,
    WHvX64RegisterRbp,
    WHvX64RegisterRsi,
    WHvX64RegisterRdi,
    WHvX64RegisterR8,
    WHvX64RegisterR9,
    WHvX64RegisterR10,
    WHvX64RegisterR11,
    WHvX64RegisterR12,
    WHvX64RegisterR13,
    WHvX64RegisterR14,
    WHvX64RegisterR15,
    WHvX64RegisterRip,
    WHvX64RegisterRflags,

    /* X64 Segment registers */
    WHvX64RegisterEs,
    WHvX64RegisterCs,
    WHvX64RegisterSs,
    WHvX64RegisterDs,
    WHvX64RegisterFs,
    WHvX64RegisterGs,
    WHvX64RegisterLdtr,
    WHvX64RegisterTr,

    /* X64 Table registers */
    WHvX64RegisterIdtr,
    WHvX64RegisterGdtr,

    /* X64 Control Registers */
    WHvX64RegisterCr0,
    WHvX64RegisterCr2,
    WHvX64RegisterCr3,
    WHvX64RegisterCr4,
    WHvX64RegisterCr8,

    /* X64 Debug Registers */
    /*
     * WHvX64RegisterDr0,
     * WHvX64RegisterDr1,
     * WHvX64RegisterDr2,
     * WHvX64RegisterDr3,
     * WHvX64RegisterDr6,
     * WHvX64RegisterDr7,
     */

    /* X64 Floating Point and Vector Registers */
    WHvX64RegisterXmm0,
    WHvX64RegisterXmm1,
    WHvX64RegisterXmm2,
    WHvX64RegisterXmm3,
    WHvX64RegisterXmm4,
    WHvX64RegisterXmm5,
    WHvX64RegisterXmm6,
    WHvX64RegisterXmm7,
    WHvX64RegisterXmm8,
    WHvX64RegisterXmm9,
    WHvX64RegisterXmm10,
    WHvX64RegisterXmm11,
    WHvX64RegisterXmm12,
    WHvX64RegisterXmm13,
    WHvX64RegisterXmm14,
    WHvX64RegisterXmm15,
    WHvX64RegisterFpMmx0,
    WHvX64RegisterFpMmx1,
    WHvX64RegisterFpMmx2,
    WHvX64RegisterFpMmx3,
    WHvX64RegisterFpMmx4,
    WHvX64RegisterFpMmx5,
    WHvX64RegisterFpMmx6,
    WHvX64RegisterFpMmx7,
    WHvX64RegisterFpControlStatus,
    WHvX64RegisterXmmControlStatus,

    /* X64 MSRs */
    WHvX64RegisterTsc,
    WHvX64RegisterEfer,
#ifdef TARGET_X86_64
    WHvX64RegisterKernelGsBase,
#endif
    WHvX64RegisterApicBase,
    /* WHvX64RegisterPat, */
    WHvX64RegisterSysenterCs,
    WHvX64RegisterSysenterEip,
    WHvX64RegisterSysenterEsp,
    WHvX64RegisterStar,
#ifdef TARGET_X86_64
    WHvX64RegisterLstar,
    WHvX64RegisterCstar,
    WHvX64RegisterSfmask,
#endif

    /* Interrupt / Event Registers */
    /*
     * WHvRegisterPendingInterruption,
     * WHvRegisterInterruptState,
     * WHvRegisterPendingEvent0,
     * WHvRegisterPendingEvent1
     * WHvX64RegisterDeliverabilityNotifications,
     */
};

struct whpx_register_set {
    WHV_REGISTER_VALUE values[RTL_NUMBER_OF(whpx_register_names)];
};

struct whpx_vcpu {
    WHV_EMULATOR_HANDLE emulator;
    bool window_registered;
    bool interruptable;
    uint64_t tpr;
    uint64_t apic_base;
    bool interruption_pending;

    /* Must be the last field as it may have a tail */
    WHV_RUN_VP_EXIT_CONTEXT exit_ctx;
};

static bool whpx_allowed;
static bool whp_dispatch_initialized = false;
static HMODULE hWinHvPlatform, hWinHvEmulation;
static WHV_PROCESSOR_XSAVE_FEATURES whpx_xsave_cap;

struct whpx_state whpx_global;
struct WHPDispatch whp_dispatch;

static bool whpx_has_xsave(void)
{
    return whpx_xsave_cap.XsaveSupport;
}

/*
 * VP support
 */

static struct whpx_vcpu *get_whpx_vcpu(CPUState *cpu)
{
    return (struct whpx_vcpu *)cpu->hax_vcpu;
}

static WHV_X64_SEGMENT_REGISTER whpx_seg_q2h(const SegmentCache *qs, int v86,
                                             int r86)
{
    WHV_X64_SEGMENT_REGISTER hs;
    unsigned flags = qs->flags;

    hs.Base = qs->base;
    hs.Limit = qs->limit;
    hs.Selector = qs->selector;

    if (v86) {
        hs.Attributes = 0;
        hs.SegmentType = 3;
        hs.Present = 1;
        hs.DescriptorPrivilegeLevel = 3;
        hs.NonSystemSegment = 1;

    } else {
        hs.Attributes = (flags >> DESC_TYPE_SHIFT);

        if (r86) {
            /* hs.Base &= 0xfffff; */
        }
    }

    return hs;
}

static SegmentCache whpx_seg_h2q(const WHV_X64_SEGMENT_REGISTER *hs)
{
    SegmentCache qs;

    qs.base = hs->Base;
    qs.limit = hs->Limit;
    qs.selector = hs->Selector;

    qs.flags = ((uint32_t)hs->Attributes) << DESC_TYPE_SHIFT;

    return qs;
}

/* X64 Extended Control Registers */
static void whpx_set_xcrs(CPUState *cpu)
{
    struct CPUX86State *env = (CPUArchState *)(cpu->env_ptr);
    HRESULT hr;
    struct whpx_state *whpx = &whpx_global;
    WHV_REGISTER_VALUE xcr0;
    WHV_REGISTER_NAME xcr0_name = WHvX64RegisterXCr0;

    if (!whpx_has_xsave()) {
        return;
    }

    /* Only xcr0 is supported by the hypervisor currently */
    xcr0.Reg64 = env->xcr0;
    hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index, &xcr0_name, 1, &xcr0);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to set register xcr0, hr=%08lx", hr);
    }
}

static void whpx_set_registers(CPUState *cpu)
{
    struct whpx_state *whpx = &whpx_global;
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);
    struct CPUX86State *env = (CPUArchState *)(cpu->env_ptr);
    X86CPU *x86_cpu = X86_CPU(cpu);
    struct whpx_register_set vcxt;
    HRESULT hr;
    int idx;
    int idx_next;
    int i;
    int v86, r86;

    assert(cpu_is_stopped(cpu) || qemu_cpu_is_self(cpu));

    memset(&vcxt, 0, sizeof(struct whpx_register_set));

    v86 = (env->eflags & VM_MASK);
    r86 = !(env->cr[0] & CR0_PE_MASK);

    vcpu->tpr = cpu_get_apic_tpr(x86_cpu->apic_state);
    vcpu->apic_base = cpu_get_apic_base(x86_cpu->apic_state);

    idx = 0;

    /* Indexes for first 16 registers match between HV and QEMU definitions */
    idx_next = 16;
    for (idx = 0; idx < CPU_NB_REGS; idx += 1) {
        vcxt.values[idx].Reg64 = (uint64_t)env->regs[idx];
    }
    idx = idx_next;

    /* Same goes for RIP and RFLAGS */
    assert(whpx_register_names[idx] == WHvX64RegisterRip);
    vcxt.values[idx++].Reg64 = env->eip;

    assert(whpx_register_names[idx] == WHvX64RegisterRflags);
    vcxt.values[idx++].Reg64 = env->eflags;

    /* Translate 6+4 segment registers. HV and QEMU order matches  */
    assert(idx == WHvX64RegisterEs);
    for (i = 0; i < 6; i += 1, idx += 1) {
        vcxt.values[idx].Segment = whpx_seg_q2h(&env->segs[i], v86, r86);
    }

    assert(idx == WHvX64RegisterLdtr);
    vcxt.values[idx++].Segment = whpx_seg_q2h(&env->ldt, 0, 0);

    assert(idx == WHvX64RegisterTr);
    vcxt.values[idx++].Segment = whpx_seg_q2h(&env->tr, 0, 0);

    assert(idx == WHvX64RegisterIdtr);
    vcxt.values[idx].Table.Base = env->idt.base;
    vcxt.values[idx].Table.Limit = env->idt.limit;
    idx += 1;

    assert(idx == WHvX64RegisterGdtr);
    vcxt.values[idx].Table.Base = env->gdt.base;
    vcxt.values[idx].Table.Limit = env->gdt.limit;
    idx += 1;

    /* CR0, 2, 3, 4, 8 */
    assert(whpx_register_names[idx] == WHvX64RegisterCr0);
    vcxt.values[idx++].Reg64 = env->cr[0];
    assert(whpx_register_names[idx] == WHvX64RegisterCr2);
    vcxt.values[idx++].Reg64 = env->cr[2];
    assert(whpx_register_names[idx] == WHvX64RegisterCr3);
    vcxt.values[idx++].Reg64 = env->cr[3];
    assert(whpx_register_names[idx] == WHvX64RegisterCr4);
    vcxt.values[idx++].Reg64 = env->cr[4];
    assert(whpx_register_names[idx] == WHvX64RegisterCr8);
    vcxt.values[idx++].Reg64 = vcpu->tpr;

    /* 8 Debug Registers - Skipped */

    /* Extended control registers */
    whpx_set_xcrs(cpu);

    /* 16 XMM registers */
    assert(whpx_register_names[idx] == WHvX64RegisterXmm0);
    idx_next = idx + 16;
    for (i = 0; i < sizeof(env->xmm_regs) / sizeof(ZMMReg); i += 1, idx += 1) {
        vcxt.values[idx].Reg128.Low64 = env->xmm_regs[i].ZMM_Q(0);
        vcxt.values[idx].Reg128.High64 = env->xmm_regs[i].ZMM_Q(1);
    }
    idx = idx_next;

    /* 8 FP registers */
    assert(whpx_register_names[idx] == WHvX64RegisterFpMmx0);
    for (i = 0; i < 8; i += 1, idx += 1) {
        vcxt.values[idx].Fp.AsUINT128.Low64 = env->fpregs[i].mmx.MMX_Q(0);
        /* vcxt.values[idx].Fp.AsUINT128.High64 =
               env->fpregs[i].mmx.MMX_Q(1);
        */
    }

    /* FP control status register */
    assert(whpx_register_names[idx] == WHvX64RegisterFpControlStatus);
    vcxt.values[idx].FpControlStatus.FpControl = env->fpuc;
    vcxt.values[idx].FpControlStatus.FpStatus =
        (env->fpus & ~0x3800) | (env->fpstt & 0x7) << 11;
    vcxt.values[idx].FpControlStatus.FpTag = 0;
    for (i = 0; i < 8; ++i) {
        vcxt.values[idx].FpControlStatus.FpTag |= (!env->fptags[i]) << i;
    }
    vcxt.values[idx].FpControlStatus.Reserved = 0;
    vcxt.values[idx].FpControlStatus.LastFpOp = env->fpop;
    vcxt.values[idx].FpControlStatus.LastFpRip = env->fpip;
    idx += 1;

    /* XMM control status register */
    assert(whpx_register_names[idx] == WHvX64RegisterXmmControlStatus);
    vcxt.values[idx].XmmControlStatus.LastFpRdp = 0;
    vcxt.values[idx].XmmControlStatus.XmmStatusControl = env->mxcsr;
    vcxt.values[idx].XmmControlStatus.XmmStatusControlMask = 0x0000ffff;
    idx += 1;

    /* MSRs */
    assert(whpx_register_names[idx] == WHvX64RegisterTsc);
    vcxt.values[idx++].Reg64 = env->tsc;
    assert(whpx_register_names[idx] == WHvX64RegisterEfer);
    vcxt.values[idx++].Reg64 = env->efer;
#ifdef TARGET_X86_64
    assert(whpx_register_names[idx] == WHvX64RegisterKernelGsBase);
    vcxt.values[idx++].Reg64 = env->kernelgsbase;
#endif

    assert(whpx_register_names[idx] == WHvX64RegisterApicBase);
    vcxt.values[idx++].Reg64 = vcpu->apic_base;

    /* WHvX64RegisterPat - Skipped */

    assert(whpx_register_names[idx] == WHvX64RegisterSysenterCs);
    vcxt.values[idx++].Reg64 = env->sysenter_cs;
    assert(whpx_register_names[idx] == WHvX64RegisterSysenterEip);
    vcxt.values[idx++].Reg64 = env->sysenter_eip;
    assert(whpx_register_names[idx] == WHvX64RegisterSysenterEsp);
    vcxt.values[idx++].Reg64 = env->sysenter_esp;
    assert(whpx_register_names[idx] == WHvX64RegisterStar);
    vcxt.values[idx++].Reg64 = env->star;
#ifdef TARGET_X86_64
    assert(whpx_register_names[idx] == WHvX64RegisterLstar);
    vcxt.values[idx++].Reg64 = env->lstar;
    assert(whpx_register_names[idx] == WHvX64RegisterCstar);
    vcxt.values[idx++].Reg64 = env->cstar;
    assert(whpx_register_names[idx] == WHvX64RegisterSfmask);
    vcxt.values[idx++].Reg64 = env->fmask;
#endif

    /* Interrupt / Event Registers - Skipped */

    assert(idx == RTL_NUMBER_OF(whpx_register_names));

    hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_register_names,
        RTL_NUMBER_OF(whpx_register_names),
        &vcxt.values[0]);

    if (FAILED(hr)) {
        error_report("WHPX: Failed to set virtual processor context, hr=%08lx",
                     hr);
    }

    return;
}

/* X64 Extended Control Registers */
static void whpx_get_xcrs(CPUState *cpu)
{
    struct CPUX86State *env = (CPUArchState *)(cpu->env_ptr);
    HRESULT hr;
    struct whpx_state *whpx = &whpx_global;
    WHV_REGISTER_VALUE xcr0;
    WHV_REGISTER_NAME xcr0_name = WHvX64RegisterXCr0;

    if (!whpx_has_xsave()) {
        return;
    }

    /* Only xcr0 is supported by the hypervisor currently */
    hr = whp_dispatch.WHvGetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index, &xcr0_name, 1, &xcr0);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to get register xcr0, hr=%08lx", hr);
        return;
    }

    env->xcr0 = xcr0.Reg64;
}

static void whpx_get_registers(CPUState *cpu)
{
    struct whpx_state *whpx = &whpx_global;
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);
    struct CPUX86State *env = (CPUArchState *)(cpu->env_ptr);
    X86CPU *x86_cpu = X86_CPU(cpu);
    struct whpx_register_set vcxt;
    uint64_t tpr, apic_base;
    HRESULT hr;
    int idx;
    int idx_next;
    int i;

    assert(cpu_is_stopped(cpu) || qemu_cpu_is_self(cpu));

    hr = whp_dispatch.WHvGetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_register_names,
        RTL_NUMBER_OF(whpx_register_names),
        &vcxt.values[0]);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to get virtual processor context, hr=%08lx",
                     hr);
    }

    idx = 0;

    /* Indexes for first 16 registers match between HV and QEMU definitions */
    idx_next = 16;
    for (idx = 0; idx < CPU_NB_REGS; idx += 1) {
        env->regs[idx] = vcxt.values[idx].Reg64;
    }
    idx = idx_next;

    /* Same goes for RIP and RFLAGS */
    assert(whpx_register_names[idx] == WHvX64RegisterRip);
    env->eip = vcxt.values[idx++].Reg64;
    assert(whpx_register_names[idx] == WHvX64RegisterRflags);
    env->eflags = vcxt.values[idx++].Reg64;

    /* Translate 6+4 segment registers. HV and QEMU order matches  */
    assert(idx == WHvX64RegisterEs);
    for (i = 0; i < 6; i += 1, idx += 1) {
        env->segs[i] = whpx_seg_h2q(&vcxt.values[idx].Segment);
    }

    assert(idx == WHvX64RegisterLdtr);
    env->ldt = whpx_seg_h2q(&vcxt.values[idx++].Segment);
    assert(idx == WHvX64RegisterTr);
    env->tr = whpx_seg_h2q(&vcxt.values[idx++].Segment);
    assert(idx == WHvX64RegisterIdtr);
    env->idt.base = vcxt.values[idx].Table.Base;
    env->idt.limit = vcxt.values[idx].Table.Limit;
    idx += 1;
    assert(idx == WHvX64RegisterGdtr);
    env->gdt.base = vcxt.values[idx].Table.Base;
    env->gdt.limit = vcxt.values[idx].Table.Limit;
    idx += 1;

    /* CR0, 2, 3, 4, 8 */
    assert(whpx_register_names[idx] == WHvX64RegisterCr0);
    env->cr[0] = vcxt.values[idx++].Reg64;
    assert(whpx_register_names[idx] == WHvX64RegisterCr2);
    env->cr[2] = vcxt.values[idx++].Reg64;
    assert(whpx_register_names[idx] == WHvX64RegisterCr3);
    env->cr[3] = vcxt.values[idx++].Reg64;
    assert(whpx_register_names[idx] == WHvX64RegisterCr4);
    env->cr[4] = vcxt.values[idx++].Reg64;
    assert(whpx_register_names[idx] == WHvX64RegisterCr8);
    tpr = vcxt.values[idx++].Reg64;
    if (tpr != vcpu->tpr) {
        vcpu->tpr = tpr;
        cpu_set_apic_tpr(x86_cpu->apic_state, tpr);
    }

    /* 8 Debug Registers - Skipped */

    /* Extended control registers */
    whpx_get_xcrs(cpu);

    /* 16 XMM registers */
    assert(whpx_register_names[idx] == WHvX64RegisterXmm0);
    idx_next = idx + 16;
    for (i = 0; i < sizeof(env->xmm_regs) / sizeof(ZMMReg); i += 1, idx += 1) {
        env->xmm_regs[i].ZMM_Q(0) = vcxt.values[idx].Reg128.Low64;
        env->xmm_regs[i].ZMM_Q(1) = vcxt.values[idx].Reg128.High64;
    }
    idx = idx_next;

    /* 8 FP registers */
    assert(whpx_register_names[idx] == WHvX64RegisterFpMmx0);
    for (i = 0; i < 8; i += 1, idx += 1) {
        env->fpregs[i].mmx.MMX_Q(0) = vcxt.values[idx].Fp.AsUINT128.Low64;
        /* env->fpregs[i].mmx.MMX_Q(1) =
               vcxt.values[idx].Fp.AsUINT128.High64;
        */
    }

    /* FP control status register */
    assert(whpx_register_names[idx] == WHvX64RegisterFpControlStatus);
    env->fpuc = vcxt.values[idx].FpControlStatus.FpControl;
    env->fpstt = (vcxt.values[idx].FpControlStatus.FpStatus >> 11) & 0x7;
    env->fpus = vcxt.values[idx].FpControlStatus.FpStatus & ~0x3800;
    for (i = 0; i < 8; ++i) {
        env->fptags[i] = !((vcxt.values[idx].FpControlStatus.FpTag >> i) & 1);
    }
    env->fpop = vcxt.values[idx].FpControlStatus.LastFpOp;
    env->fpip = vcxt.values[idx].FpControlStatus.LastFpRip;
    idx += 1;

    /* XMM control status register */
    assert(whpx_register_names[idx] == WHvX64RegisterXmmControlStatus);
    env->mxcsr = vcxt.values[idx].XmmControlStatus.XmmStatusControl;
    idx += 1;

    /* MSRs */
    assert(whpx_register_names[idx] == WHvX64RegisterTsc);
    env->tsc = vcxt.values[idx++].Reg64;
    assert(whpx_register_names[idx] == WHvX64RegisterEfer);
    env->efer = vcxt.values[idx++].Reg64;
#ifdef TARGET_X86_64
    assert(whpx_register_names[idx] == WHvX64RegisterKernelGsBase);
    env->kernelgsbase = vcxt.values[idx++].Reg64;
#endif

    assert(whpx_register_names[idx] == WHvX64RegisterApicBase);
    apic_base = vcxt.values[idx++].Reg64;
    if (apic_base != vcpu->apic_base) {
        vcpu->apic_base = apic_base;
        cpu_set_apic_base(x86_cpu->apic_state, vcpu->apic_base);
    }

    /* WHvX64RegisterPat - Skipped */

    assert(whpx_register_names[idx] == WHvX64RegisterSysenterCs);
    env->sysenter_cs = vcxt.values[idx++].Reg64;;
    assert(whpx_register_names[idx] == WHvX64RegisterSysenterEip);
    env->sysenter_eip = vcxt.values[idx++].Reg64;
    assert(whpx_register_names[idx] == WHvX64RegisterSysenterEsp);
    env->sysenter_esp = vcxt.values[idx++].Reg64;
    assert(whpx_register_names[idx] == WHvX64RegisterStar);
    env->star = vcxt.values[idx++].Reg64;
#ifdef TARGET_X86_64
    assert(whpx_register_names[idx] == WHvX64RegisterLstar);
    env->lstar = vcxt.values[idx++].Reg64;
    assert(whpx_register_names[idx] == WHvX64RegisterCstar);
    env->cstar = vcxt.values[idx++].Reg64;
    assert(whpx_register_names[idx] == WHvX64RegisterSfmask);
    env->fmask = vcxt.values[idx++].Reg64;
#endif

    /* Interrupt / Event Registers - Skipped */

    assert(idx == RTL_NUMBER_OF(whpx_register_names));

    return;
}

static HRESULT CALLBACK whpx_emu_ioport_callback(
    void *ctx,
    WHV_EMULATOR_IO_ACCESS_INFO *IoAccess)
{
    MemTxAttrs attrs = { 0 };
    address_space_rw(&address_space_io, IoAccess->Port, attrs,
                     (uint8_t *)&IoAccess->Data, IoAccess->AccessSize,
                     IoAccess->Direction);
    return S_OK;
}

static HRESULT CALLBACK whpx_emu_mmio_callback(
    void *ctx,
    WHV_EMULATOR_MEMORY_ACCESS_INFO *ma)
{
    cpu_physical_memory_rw(ma->GpaAddress, ma->Data, ma->AccessSize,
                           ma->Direction);
    return S_OK;
}

static HRESULT CALLBACK whpx_emu_getreg_callback(
    void *ctx,
    const WHV_REGISTER_NAME *RegisterNames,
    UINT32 RegisterCount,
    WHV_REGISTER_VALUE *RegisterValues)
{
    HRESULT hr;
    struct whpx_state *whpx = &whpx_global;
    CPUState *cpu = (CPUState *)ctx;

    hr = whp_dispatch.WHvGetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        RegisterNames, RegisterCount,
        RegisterValues);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to get virtual processor registers,"
                     " hr=%08lx", hr);
    }

    return hr;
}

static HRESULT CALLBACK whpx_emu_setreg_callback(
    void *ctx,
    const WHV_REGISTER_NAME *RegisterNames,
    UINT32 RegisterCount,
    const WHV_REGISTER_VALUE *RegisterValues)
{
    HRESULT hr;
    struct whpx_state *whpx = &whpx_global;
    CPUState *cpu = (CPUState *)ctx;

    hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        RegisterNames, RegisterCount,
        RegisterValues);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to set virtual processor registers,"
                     " hr=%08lx", hr);
    }

    /*
     * The emulator just successfully wrote the register state. We clear the
     * dirty state so we avoid the double write on resume of the VP.
     */
    cpu->vcpu_dirty = false;

    return hr;
}

static HRESULT CALLBACK whpx_emu_translate_callback(
    void *ctx,
    WHV_GUEST_VIRTUAL_ADDRESS Gva,
    WHV_TRANSLATE_GVA_FLAGS TranslateFlags,
    WHV_TRANSLATE_GVA_RESULT_CODE *TranslationResult,
    WHV_GUEST_PHYSICAL_ADDRESS *Gpa)
{
    HRESULT hr;
    struct whpx_state *whpx = &whpx_global;
    CPUState *cpu = (CPUState *)ctx;
    WHV_TRANSLATE_GVA_RESULT res;

    hr = whp_dispatch.WHvTranslateGva(whpx->partition, cpu->cpu_index,
                                      Gva, TranslateFlags, &res, Gpa);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to translate GVA, hr=%08lx", hr);
    } else {
        *TranslationResult = res.ResultCode;
    }

    return hr;
}

static const WHV_EMULATOR_CALLBACKS whpx_emu_callbacks = {
    .Size = sizeof(WHV_EMULATOR_CALLBACKS),
    .WHvEmulatorIoPortCallback = whpx_emu_ioport_callback,
    .WHvEmulatorMemoryCallback = whpx_emu_mmio_callback,
    .WHvEmulatorGetVirtualProcessorRegisters = whpx_emu_getreg_callback,
    .WHvEmulatorSetVirtualProcessorRegisters = whpx_emu_setreg_callback,
    .WHvEmulatorTranslateGvaPage = whpx_emu_translate_callback,
};

static int whpx_handle_mmio(CPUState *cpu, WHV_MEMORY_ACCESS_CONTEXT *ctx)
{
    HRESULT hr;
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);
    struct whpx_vcpu vcpu_copy = *vcpu;
    WHV_MEMORY_ACCESS_CONTEXT ctx_copy = *ctx;
    WHV_EMULATOR_STATUS emu_status;

    hr = whp_dispatch.WHvEmulatorTryMmioEmulation(
        vcpu->emulator, cpu,
        &vcpu->exit_ctx.VpContext, ctx,
        &emu_status);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to parse MMIO access, hr=%08lx", hr);
        return -1;
    }

    if (!emu_status.EmulationSuccessful) {
        if (emu_status.GuestCannotBeFaulted) {
            /* Resume the VP. This is a workaround for a WHP bug until the
             * fix is available in the OS.
             */
            printf("WHPX: Ignoring expected 'GuestCannotBeFaulted' error "
                   "while handling MMIO access.\n");
            return 0;
        }

        /* each byte takes 3 characters to print: "XX " */
        char instruction[sizeof(ctx_copy.InstructionBytes) * 3 + 1];
        int i;
        int size;

        error_report("WHPX: Failed to emulate MMIO access with"
                     " EmulatorReturnStatus: %u (%s%s%s%s%s%s%s%s%s%s)",
            emu_status.AsUINT32,
            (emu_status.EmulationSuccessful ? "EmulationSuccessful " : ""),
            (emu_status.InternalEmulationFailure ?
                "InternalEmulationFailure " : ""),
            (emu_status.IoPortCallbackFailed ? "IoPortCallbackFailed " : ""),
            (emu_status.MemoryCallbackFailed ? "MemoryCallbackFailed " : ""),
            (emu_status.TranslateGvaPageCallbackFailed ?
                "TranslateGvaPageCallbackFailed " : ""),
            (emu_status.TranslateGvaPageCallbackGpaIsNotAligned ?
                "TranslateGvaPageCallbackGpaIsNotAligned " : ""),
            (emu_status.GetVirtualProcessorRegistersCallbackFailed ?
                "GetVirtualProcessorRegistersCallbackFailed " : ""),
            (emu_status.SetVirtualProcessorRegistersCallbackFailed ?
                "SetVirtualProcessorRegistersCallbackFailed " : ""),
            (emu_status.InterruptCausedIntercept ?
                "InterruptCausedIntercept " : ""),
            (emu_status.GuestCannotBeFaulted ? "GuestCannotBeFaulted " : ""));

        error_report("whpx_vcpu { emulator=%p, window_registered=%s, "
                     "interruptable=%s, tpr=%llx, apic_base=%llx, "
                     "interruption_pending=%s }",
            vcpu_copy.emulator,
            (vcpu_copy.window_registered ? "true" : "false"),
            (vcpu_copy.interruptable ? "true" : "false"),
            vcpu_copy.tpr,
            vcpu_copy.apic_base,
            (vcpu_copy.interruption_pending ? "true" : "false"));

        size = MIN(ctx_copy.InstructionByteCount,
                   sizeof(ctx_copy.InstructionBytes));
        for (i = 0; i < size; ++i) {
            sprintf(&instruction[i * 3], "%02X ", ctx_copy.InstructionBytes[i]);
        }

        error_report("WHV_MEMORY_ACCESS_CONTEXT { "
                     "Instruction={ size=%u, bytes='%s%s' }, "
                     "AccessInfo={ AccessType=%u, GpaUnmapped=%u, "
                     "GvaValid=%u, AsUINT32=%u }, Gpa=%llx, Gva=%llx }",
            ctx_copy.InstructionByteCount,
            instruction,
            (ctx_copy.InstructionByteCount >
                sizeof(ctx_copy.InstructionBytes) ? "..." : ""),
            ctx_copy.AccessInfo.AccessType,
            ctx_copy.AccessInfo.GpaUnmapped,
            ctx_copy.AccessInfo.GvaValid,
            ctx_copy.AccessInfo.AsUINT32,
            ctx_copy.Gpa,
            ctx_copy.Gva);

        return -1;
    }

    return 0;
}

static int whpx_handle_portio(CPUState *cpu,
                              WHV_X64_IO_PORT_ACCESS_CONTEXT *ctx)
{
    HRESULT hr;
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);
    WHV_EMULATOR_STATUS emu_status;

    hr = whp_dispatch.WHvEmulatorTryIoEmulation(
        vcpu->emulator, cpu,
        &vcpu->exit_ctx.VpContext, ctx,
        &emu_status);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to parse PortIO access, hr=%08lx", hr);
        return -1;
    }

    if (!emu_status.EmulationSuccessful) {
        error_report("WHPX: Failed to emulate PortIO access with"
                     " EmulatorReturnStatus: %u", emu_status.AsUINT32);
        return -1;
    }

    return 0;
}

static int whpx_handle_halt(CPUState *cpu)
{
    struct CPUX86State *env = (CPUArchState *)(cpu->env_ptr);
    int ret = 0;

    qemu_mutex_lock_iothread();
    if (!((cpu->interrupt_request & CPU_INTERRUPT_HARD) &&
          (env->eflags & IF_MASK)) &&
        !(cpu->interrupt_request & CPU_INTERRUPT_NMI)) {
        cpu->exception_index = EXCP_HLT;
        cpu->halted = true;
        ret = 1;
    }
    qemu_mutex_unlock_iothread();

    return ret;
}

static void whpx_vcpu_pre_run(CPUState *cpu)
{
    HRESULT hr;
    struct whpx_state *whpx = &whpx_global;
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);
    struct CPUX86State *env = (CPUArchState *)(cpu->env_ptr);
    X86CPU *x86_cpu = X86_CPU(cpu);
    int irq;
    uint8_t tpr;
    WHV_X64_PENDING_INTERRUPTION_REGISTER new_int;
    UINT32 reg_count = 0;
    WHV_REGISTER_VALUE reg_values[3];
    WHV_REGISTER_NAME reg_names[3];

    memset(&new_int, 0, sizeof(new_int));
    memset(reg_values, 0, sizeof(reg_values));

    qemu_mutex_lock_iothread();

    /* Inject NMI */
    if (!vcpu->interruption_pending &&
        cpu->interrupt_request & (CPU_INTERRUPT_NMI | CPU_INTERRUPT_SMI)) {
        if (cpu->interrupt_request & CPU_INTERRUPT_NMI) {
            cpu->interrupt_request &= ~CPU_INTERRUPT_NMI;
            vcpu->interruptable = false;
            new_int.InterruptionType = WHvX64PendingNmi;
            new_int.InterruptionPending = 1;
            new_int.InterruptionVector = 2;
        }
        if (cpu->interrupt_request & CPU_INTERRUPT_SMI) {
            cpu->interrupt_request &= ~CPU_INTERRUPT_SMI;
        }
    }

    /*
     * Force the VCPU out of its inner loop to process any INIT requests or
     * commit pending TPR access.
     */
    if (cpu->interrupt_request & (CPU_INTERRUPT_INIT | CPU_INTERRUPT_TPR)) {
        if ((cpu->interrupt_request & CPU_INTERRUPT_INIT) &&
            !(env->hflags & HF_SMM_MASK)) {
            cpu->exit_request = 1;
        }
        if (cpu->interrupt_request & CPU_INTERRUPT_TPR) {
            cpu->exit_request = 1;
        }
    }

    /* Get pending hard interruption or replay one that was overwritten */
    if (!vcpu->interruption_pending &&
        vcpu->interruptable && (env->eflags & IF_MASK)) {
        assert(!new_int.InterruptionPending);
        if (cpu->interrupt_request & CPU_INTERRUPT_HARD) {
            cpu->interrupt_request &= ~CPU_INTERRUPT_HARD;
            irq = cpu_get_pic_interrupt(env);
            if (irq >= 0) {
                new_int.InterruptionType = WHvX64PendingInterrupt;
                new_int.InterruptionPending = 1;
                new_int.InterruptionVector = irq;
            }
        }
    }

    /* Setup interrupt state if new one was prepared */
    if (new_int.InterruptionPending) {
        reg_values[reg_count].PendingInterruption = new_int;
        reg_names[reg_count] = WHvRegisterPendingInterruption;
        reg_count += 1;
    }

    /* Sync the TPR to the CR8 if was modified during the intercept */
    tpr = cpu_get_apic_tpr(x86_cpu->apic_state);
    if (tpr != vcpu->tpr) {
        vcpu->tpr = tpr;
        reg_values[reg_count].Reg64 = tpr;
        cpu->exit_request = 1;
        reg_names[reg_count] = WHvX64RegisterCr8;
        reg_count += 1;
    }

    /* Update the state of the interrupt delivery notification */
    if (!vcpu->window_registered &&
        cpu->interrupt_request & CPU_INTERRUPT_HARD) {
        reg_values[reg_count].DeliverabilityNotifications.InterruptNotification
            = 1;
        vcpu->window_registered = 1;
        reg_names[reg_count] = WHvX64RegisterDeliverabilityNotifications;
        reg_count += 1;
    }

    qemu_mutex_unlock_iothread();

    if (reg_count) {
        hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
            whpx->partition, cpu->cpu_index,
            reg_names, reg_count, reg_values);
        if (FAILED(hr)) {
            error_report("WHPX: Failed to set interrupt state registers,"
                         " hr=%08lx", hr);
        }
    }

    return;
}

static void whpx_vcpu_post_run(CPUState *cpu)
{
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);
    struct CPUX86State *env = (CPUArchState *)(cpu->env_ptr);
    X86CPU *x86_cpu = X86_CPU(cpu);
    env->eflags = vcpu->exit_ctx.VpContext.Rflags;

    uint64_t tpr = vcpu->exit_ctx.VpContext.Cr8;
    if (vcpu->tpr != tpr) {
        vcpu->tpr = tpr;
        qemu_mutex_lock_iothread();
        cpu_set_apic_tpr(x86_cpu->apic_state, vcpu->tpr);
        qemu_mutex_unlock_iothread();
    }

    vcpu->interruption_pending =
        vcpu->exit_ctx.VpContext.ExecutionState.InterruptionPending;

    vcpu->interruptable =
        !vcpu->exit_ctx.VpContext.ExecutionState.InterruptShadow;

    return;
}

static void whpx_vcpu_process_async_events(CPUState *cpu)
{
    struct CPUX86State *env = (CPUArchState *)(cpu->env_ptr);
    X86CPU *x86_cpu = X86_CPU(cpu);
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);

    if ((cpu->interrupt_request & CPU_INTERRUPT_INIT) &&
        !(env->hflags & HF_SMM_MASK)) {

        do_cpu_init(x86_cpu);
        cpu->vcpu_dirty = true;
        vcpu->interruptable = true;
    }

    if (cpu->interrupt_request & CPU_INTERRUPT_POLL) {
        cpu->interrupt_request &= ~CPU_INTERRUPT_POLL;
        apic_poll_irq(x86_cpu->apic_state);
    }

    if (((cpu->interrupt_request & CPU_INTERRUPT_HARD) &&
         (env->eflags & IF_MASK)) ||
        (cpu->interrupt_request & CPU_INTERRUPT_NMI)) {
        cpu->halted = false;
    }

    if (cpu->interrupt_request & CPU_INTERRUPT_SIPI) {
        if (!cpu->vcpu_dirty) {
            whpx_get_registers(cpu);
        }
        do_cpu_sipi(x86_cpu);
    }

    if (cpu->interrupt_request & CPU_INTERRUPT_TPR) {
        cpu->interrupt_request &= ~CPU_INTERRUPT_TPR;
        if (!cpu->vcpu_dirty) {
            whpx_get_registers(cpu);
        }
        apic_handle_tpr_access_report(x86_cpu->apic_state, env->eip,
                                      env->tpr_access_type);
    }

    return;
}

static int whpx_vcpu_run(CPUState *cpu)
{
    HRESULT hr;
    struct whpx_state *whpx = &whpx_global;
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);
    int ret;

    whpx_vcpu_process_async_events(cpu);
    if (cpu->halted) {
        cpu->exception_index = EXCP_HLT;
        atomic_set(&cpu->exit_request, false);
        return 0;
    }

    qemu_mutex_unlock_iothread();
    cpu_exec_start(cpu);

    do {
        if (cpu->vcpu_dirty) {
            whpx_set_registers(cpu);
            cpu->vcpu_dirty = false;
        }

        whpx_vcpu_pre_run(cpu);

        if (atomic_read(&cpu->exit_request)) {
            whpx_vcpu_kick(cpu);
        }

        hr = whp_dispatch.WHvRunVirtualProcessor(
            whpx->partition, cpu->cpu_index,
            &vcpu->exit_ctx, sizeof(vcpu->exit_ctx));

        if (FAILED(hr)) {
            error_report("WHPX: Failed to exec a virtual processor,"
                         " hr=%08lx", hr);
            ret = -1;
            break;
        }

        whpx_vcpu_post_run(cpu);

        switch (vcpu->exit_ctx.ExitReason) {
        case WHvRunVpExitReasonMemoryAccess:
            ret = whpx_handle_mmio(cpu, &vcpu->exit_ctx.MemoryAccess);
            break;

        case WHvRunVpExitReasonX64IoPortAccess:
            ret = whpx_handle_portio(cpu, &vcpu->exit_ctx.IoPortAccess);
            break;

        case WHvRunVpExitReasonX64InterruptWindow:
            vcpu->window_registered = 0;
            ret = 0;
            break;

        case WHvRunVpExitReasonX64Halt:
            ret = whpx_handle_halt(cpu);
            break;

        case WHvRunVpExitReasonCanceled:
            cpu->exception_index = EXCP_INTERRUPT;
            ret = 1;
            break;

        case WHvRunVpExitReasonX64MsrAccess: {
            WHV_REGISTER_VALUE reg_values[3] = {0};
            WHV_REGISTER_NAME reg_names[3];
            UINT32 reg_count;

            reg_names[0] = WHvX64RegisterRip;
            reg_names[1] = WHvX64RegisterRax;
            reg_names[2] = WHvX64RegisterRdx;

            reg_values[0].Reg64 =
                vcpu->exit_ctx.VpContext.Rip +
                vcpu->exit_ctx.VpContext.InstructionLength;

            /*
             * For all unsupported MSR access we:
             *     ignore writes
             *     return 0 on read.
             */
            reg_count = vcpu->exit_ctx.MsrAccess.AccessInfo.IsWrite ?
                        1 : 3;

            hr = whp_dispatch.WHvSetVirtualProcessorRegisters(whpx->partition,
                                                 cpu->cpu_index,
                                                 reg_names, reg_count,
                                                 reg_values);

            if (FAILED(hr)) {
                error_report("WHPX: Failed to set MsrAccess state "
                             " registers, hr=%08lx", hr);
            }
            ret = 0;
            break;
        }
        case WHvRunVpExitReasonX64Cpuid: {
            WHV_REGISTER_VALUE reg_values[5];
            WHV_REGISTER_NAME reg_names[5];
            UINT32 reg_count = 5;
            UINT64 rip, rax = 0, rcx = 0, rdx = 0, rbx = 0;
            UINT32 signature[3] = {0};
            struct CPUX86State *env = (CPUArchState *)(cpu->env_ptr);

            memset(reg_values, 0, sizeof(reg_values));

            rip = vcpu->exit_ctx.VpContext.Rip +
                  vcpu->exit_ctx.VpContext.InstructionLength;

            cpu_x86_cpuid(env, vcpu->exit_ctx.CpuidAccess.Rax,
                               vcpu->exit_ctx.CpuidAccess.Rcx,
                               &rax, &rbx, &rcx, &rdx);

            switch (vcpu->exit_ctx.CpuidAccess.Rax) {
                case 0x80000001:
                    /* Remove any support of OSVW */
                    rcx &= ~CPUID_EXT3_OSVW;
                    break;
                default:
                    break;
            }

            reg_names[0] = WHvX64RegisterRip;
            reg_names[1] = WHvX64RegisterRax;
            reg_names[2] = WHvX64RegisterRcx;
            reg_names[3] = WHvX64RegisterRdx;
            reg_names[4] = WHvX64RegisterRbx;

            reg_values[0].Reg64 = rip;
            reg_values[1].Reg64 = rax;
            reg_values[2].Reg64 = rcx;
            reg_values[3].Reg64 = rdx;
            reg_values[4].Reg64 = rbx;

            hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
                whpx->partition, cpu->cpu_index,
                reg_names,
                reg_count,
                reg_values);

            if (FAILED(hr)) {
                error_report("WHPX: Failed to set CpuidAccess state registers,"
                             " hr=%08lx", hr);
            }
            ret = 0;
            break;
        }
        case WHvRunVpExitReasonNone:
        case WHvRunVpExitReasonUnrecoverableException:
        case WHvRunVpExitReasonInvalidVpRegisterValue:
        case WHvRunVpExitReasonUnsupportedFeature:
        case WHvRunVpExitReasonException:
        default:
            error_report("WHPX: Unexpected VP exit code %d",
                         vcpu->exit_ctx.ExitReason);
            whpx_get_registers(cpu);
            qemu_mutex_lock_iothread();
            qemu_system_guest_panicked(cpu_get_crash_info(cpu));
            qemu_mutex_unlock_iothread();
            break;
        }

    } while (!ret);

    cpu_exec_end(cpu);
    qemu_mutex_lock_iothread();
    current_cpu = cpu;

    atomic_set(&cpu->exit_request, false);

    return ret < 0;
}

static void do_whpx_cpu_synchronize_state(CPUState *cpu, run_on_cpu_data arg)
{
    whpx_get_registers(cpu);
    cpu->vcpu_dirty = true;
}

static void do_whpx_cpu_synchronize_post_reset(CPUState *cpu,
                                               run_on_cpu_data arg)
{
    whpx_set_registers(cpu);
    cpu->vcpu_dirty = false;
}

static void do_whpx_cpu_synchronize_post_init(CPUState *cpu,
                                              run_on_cpu_data arg)
{
    whpx_set_registers(cpu);
    cpu->vcpu_dirty = false;
}

static void do_whpx_cpu_synchronize_pre_loadvm(CPUState *cpu,
                                               run_on_cpu_data arg)
{
    cpu->vcpu_dirty = true;
}

/*
 * CPU support.
 */

void whpx_cpu_synchronize_state(CPUState *cpu)
{
    if (!cpu->vcpu_dirty) {
        run_on_cpu(cpu, do_whpx_cpu_synchronize_state, RUN_ON_CPU_NULL);
    }
}

void whpx_cpu_synchronize_post_reset(CPUState *cpu)
{
    run_on_cpu(cpu, do_whpx_cpu_synchronize_post_reset, RUN_ON_CPU_NULL);
}

void whpx_cpu_synchronize_post_init(CPUState *cpu)
{
    run_on_cpu(cpu, do_whpx_cpu_synchronize_post_init, RUN_ON_CPU_NULL);
}

void whpx_cpu_synchronize_pre_loadvm(CPUState *cpu)
{
    run_on_cpu(cpu, do_whpx_cpu_synchronize_pre_loadvm, RUN_ON_CPU_NULL);
}

/*
 * Vcpu support.
 */

// static Error *whpx_migration_blocker;

int whpx_init_vcpu(CPUState *cpu)
{
    HRESULT hr;
    struct whpx_state *whpx = &whpx_global;
    struct whpx_vcpu *vcpu;
    Error *local_error = NULL;

    // We temporarily disable the migration blocker, allowing emulator
    // Quick Boot to perform. This shouldn't be an issue, unless the
    // snapshot image is moved to a different host.
    //
    /* Add migration blockers for all unsupported features of the
     * Windows Hypervisor Platform
     */
    // if (whpx_migration_blocker == NULL) {
    //     error_setg(&whpx_migration_blocker,
    //            "State blocked due to non-migratable CPUID feature support,"
    //            "dirty memory tracking support, and XSAVE/XRSTOR support");

    //     (void)migrate_add_blocker(whpx_migration_blocker, &local_error);
    //     if (local_error) {
    //         error_report_err(local_error);
    //         migrate_del_blocker(whpx_migration_blocker);
    //         error_free(whpx_migration_blocker);
    //         return -EINVAL;
    //     }
    // }

    vcpu = g_malloc0(sizeof(struct whpx_vcpu));

    if (!vcpu) {
        error_report("WHPX: Failed to allocte VCPU context.");
        return -ENOMEM;
    }

    hr = whp_dispatch.WHvEmulatorCreateEmulator(&whpx_emu_callbacks, &vcpu->emulator);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to setup instruction completion support,"
                     " hr=%08lx", hr);
        g_free(vcpu);
        return -EINVAL;
    }

    hr = whp_dispatch.WHvCreateVirtualProcessor(whpx->partition, cpu->cpu_index, 0);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to create a virtual processor,"
                     " hr=%08lx", hr);
        whp_dispatch.WHvEmulatorDestroyEmulator(vcpu->emulator);
        g_free(vcpu);
        return -EINVAL;
    }

    vcpu->interruptable = true;

    cpu->vcpu_dirty = true;
    cpu->hax_vcpu = (struct hax_vcpu_state *)vcpu;

    return 0;
}

int whpx_vcpu_exec(CPUState *cpu)
{
    int ret;
    int fatal;

    for (;;) {
        if (cpu->exception_index >= EXCP_INTERRUPT) {
            ret = cpu->exception_index;
            cpu->exception_index = -1;
            break;
        }

        fatal = whpx_vcpu_run(cpu);

        if (fatal) {
            error_report("WHPX: Failed to exec a virtual processor");
            abort();
        }
    }

    return ret;
}

void whpx_destroy_vcpu(CPUState *cpu)
{
    struct whpx_state *whpx = &whpx_global;
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);

    whp_dispatch.WHvDeleteVirtualProcessor(whpx->partition, cpu->cpu_index);
    whp_dispatch.WHvEmulatorDestroyEmulator(vcpu->emulator);
    g_free(cpu->hax_vcpu);
    return;
}

void whpx_vcpu_kick(CPUState *cpu)
{
    struct whpx_state *whpx = &whpx_global;
    whp_dispatch.WHvCancelRunVirtualProcessor(whpx->partition, cpu->cpu_index, 0);
}

/*
 * Memory support.
 */

#define WHPX_MAX_SLOTS 64

struct whpx_slot {
    int present;
    uint64_t size;
    uint64_t gpa_start;
    uint64_t gva;
    void* hva;
};

struct whpx_slot whpx_slots[WHPX_MAX_SLOTS];

void* whpx_gpa2hva(uint64_t gpa, bool* found) {
    struct whpx_slot *mslot;
    *found = false;
    uint32_t i;

    for (i = 0; i < WHPX_MAX_SLOTS; i++) {
        mslot = &whpx_slots[i];
        if (!mslot->size) continue;
        if (gpa >= mslot->gpa_start &&
            gpa < mslot->gpa_start + mslot->size) {
            *found = true;
            return (void*)((char*)(mslot->hva) + (gpa - mslot->gpa_start));
        }
    }

    return 0;
}

struct whpx_slot* whpx_next_free_slot() {
    struct whpx_slot *mem = 0;
    int x;

    for (x = 0; x < WHPX_MAX_SLOTS; ++x) {
        mem = whpx_slots + x;
        if (!mem->size) return mem;
    }

    return mem;
}

struct whpx_slot* whpx_find_overlap_slot(uint64_t start, uint64_t end, bool* coincident) {
    struct whpx_slot *slot = 0;
    int x;

    if (coincident) *coincident = false;

    for (x = 0; x < WHPX_MAX_SLOTS; ++x) {
        slot = whpx_slots + x;
        if (slot->size && start < (slot->gpa_start + slot->size) && end > slot->gpa_start) {
            // Is [start, end) describing the same range?
            if ((start == slot->gpa_start) &&
                (end == slot->gpa_start + slot->size)) {
                if (coincident) *coincident = true;
            }
            return slot;
        }
    }

    return slot;
}

static HRESULT whpx_set_ram(
    VOID* SourceAddress,
    WHV_GUEST_PHYSICAL_ADDRESS GuestAddress,
    UINT64 SizeInBytes,
    WHV_MAP_GPA_RANGE_FLAGS Flags,
    int add) {

    struct whpx_state *whpx = &whpx_global;
    HRESULT hr;

    WHV_PARTITION_HANDLE partition = whpx->partition;

    if (add) {
        hr = whp_dispatch.WHvMapGpaRange(partition,
                                         SourceAddress,
                                         GuestAddress,
                                         SizeInBytes,
                                         Flags);
    } else {
        hr = whp_dispatch.WHvUnmapGpaRange(partition,
                                           GuestAddress,
                                           SizeInBytes);
    }

    if (FAILED(hr)) {
        error_report("WHPX: Failed to %s GPA range PA:%p, Size:%p bytes,"
                     " Host:%p, hr=%08lx",
                     (add ? "MAP" : "UNMAP"),
                     (void *)(uintptr_t)GuestAddress, (void *)SizeInBytes, SourceAddress, hr);
        return hr;
    }

    // Success. Update our internal slots.
    {
        // Could have overlapped with existing slot.
        // If so, and not coincident, reset the old mapping.
        bool coincident;
        struct whpx_slot* overlap_slot =
            whpx_find_overlap_slot(
                GuestAddress,
                GuestAddress + SizeInBytes,
                &coincident);

        if (overlap_slot && coincident) {
            if (add) {
                // Nothing to do because the slot overlaps, return.
                return hr;
            } else {
                // Coincident overlap slot; delete
                overlap_slot->size = 0;
                return hr;
            }
        }

        // Delete existing overlap slot, and add the new one to a free slot,
        // if we are adding a new region.
        if (overlap_slot) {
            overlap_slot->size = 0;
        }

        if (add) {
            struct whpx_slot* new_slot = 0;
            new_slot = whpx_next_free_slot();

            if (!new_slot) {
                qemu_abort("%s: no free slots\n", __func__);
            }

            new_slot->size = SizeInBytes;
            new_slot->gpa_start = GuestAddress;
            new_slot->hva = SourceAddress;
        }
    }

    return hr;
}

static void whpx_update_mapping(hwaddr start_pa, ram_addr_t size,
                                void *host_va, int add, int rom,
                                const char *name)
{
    struct whpx_state *whpx = &whpx_global;

    /*
    if (add) {
        printf("WHPX: ADD PA:%p Size:%p, Host:%p, %s, '%s'\n",
               (void*)start_pa, (void*)size, host_va,
               (rom ? "ROM" : "RAM"), name);
    } else {
        printf("WHPX: DEL PA:%p Size:%p, Host:%p,      '%s'\n",
               (void*)start_pa, (void*)size, host_va, name);
    }
    */

    whpx_set_ram(host_va,
                 start_pa,
                 size,
                 (WHvMapGpaRangeFlagRead |
                  WHvMapGpaRangeFlagExecute |
                  (rom ? 0 : WHvMapGpaRangeFlagWrite)),
                 add);
}

static void whpx_process_section(MemoryRegionSection *section, int add)
{
    MemoryRegion *mr = section->mr;
    hwaddr start_pa = section->offset_within_address_space;
    ram_addr_t size = int128_get64(section->size);
    unsigned int delta;
    uint64_t host_va;

    if (!memory_region_is_ram(mr)) {
        return;
    }

    if (memory_region_is_user_backed(mr)) {
        return;
    }

    delta = qemu_real_host_page_size - (start_pa & ~qemu_real_host_page_mask);
    delta &= ~qemu_real_host_page_mask;
    if (delta > size) {
        return;
    }
    start_pa += delta;
    size -= delta;
    size &= qemu_real_host_page_mask;
    if (!size || (start_pa & ~qemu_real_host_page_mask)) {
        return;
    }

    host_va = (uintptr_t)memory_region_get_ram_ptr(mr)
            + section->offset_within_region + delta;

    whpx_update_mapping(start_pa, size, (void *)(uintptr_t)host_va, add,
                        memory_region_is_rom(mr), mr->name);
}

// User backed memory region API
static WHV_MAP_GPA_RANGE_FLAGS
user_backed_flags_to_whpx(int flags) {
    WHV_MAP_GPA_RANGE_FLAGS whpx_flags = 0;
    if (flags & USER_BACKED_RAM_FLAGS_READ) {
        whpx_flags |= WHvMapGpaRangeFlagRead;
    }
    if (flags & USER_BACKED_RAM_FLAGS_WRITE) {
        whpx_flags |= WHvMapGpaRangeFlagWrite;
    }
    if (flags & USER_BACKED_RAM_FLAGS_EXEC) {
        whpx_flags |= WHvMapGpaRangeFlagExecute;
    }
    return whpx_flags;
}

static void whpx_user_backed_ram_map(uint64_t gpa, void* hva, uint64_t size, int flags) {
    struct whpx_state *whpx = &whpx_global;
    HRESULT hr;

    hr = whpx_set_ram(hva, gpa, size, user_backed_flags_to_whpx(flags), 1 /* add */);

    if (FAILED(hr)) {
        error_report(
            "WHPX: Failed to map GPA range "
            "[0x%llx 0x%llx] to HVA %p. flags: 0x%x",
            (unsigned long long)gpa,
            (unsigned long long)(gpa + size),
            hva, flags);
    }
}

static void whpx_user_backed_ram_unmap(uint64_t gpa, uint64_t size) {
    struct whpx_state *whpx = &whpx_global;
    HRESULT hr;

    hr = whpx_set_ram(0, gpa, size, 0 /* no flags */, 0 /* for delete */);

    if (FAILED(hr)) {
        error_report(
            "WHPX: Failed to unmap GPA range "
            "[0x%llx 0x%llx]",
            (unsigned long long)gpa,
            (unsigned long long)(gpa + size));
    }
}

static void whpx_region_add(MemoryListener *listener,
                           MemoryRegionSection *section)
{
    memory_region_ref(section->mr);
    whpx_process_section(section, 1);
}

static void whpx_region_del(MemoryListener *listener,
                           MemoryRegionSection *section)
{
    whpx_process_section(section, 0);
    memory_region_unref(section->mr);
}

static void whpx_transaction_begin(MemoryListener *listener)
{
}

static void whpx_transaction_commit(MemoryListener *listener)
{
}

static void whpx_log_sync(MemoryListener *listener,
                         MemoryRegionSection *section)
{
    MemoryRegion *mr = section->mr;

    if (!memory_region_is_ram(mr)) {
        return;
    }

    memory_region_set_dirty(mr, 0, int128_get64(section->size));
}

static MemoryListener whpx_memory_listener = {
    .begin = whpx_transaction_begin,
    .commit = whpx_transaction_commit,
    .region_add = whpx_region_add,
    .region_del = whpx_region_del,
    .log_sync = whpx_log_sync,
    .priority = 10,
};

static void whpx_memory_init(void)
{
    memory_listener_register(&whpx_memory_listener, &address_space_memory);
    qemu_set_user_backed_mapping_funcs(
        whpx_user_backed_ram_map,
        whpx_user_backed_ram_unmap);
}

static void whpx_handle_interrupt(CPUState *cpu, int mask)
{
    cpu->interrupt_request |= mask;

    if (!qemu_cpu_is_self(cpu)) {
        qemu_cpu_kick(cpu);
    }
}

/*
 * Partition support
 */
static void whpx_disable_xsave(struct whpx_state *whpx)
{
    WHV_PROCESSOR_XSAVE_FEATURES xsave_cap;
    HRESULT hr;

    typedef int(CALLBACK* PFN_RTLGETVERSION) (PRTL_OSVERSIONINFOW lpVersionInformation);
    PFN_RTLGETVERSION pRtlGetVersion;
    HMODULE hDll = LoadLibrary("Ntdll.dll");
    pRtlGetVersion = (PFN_RTLGETVERSION)GetProcAddress(hDll, "RtlGetVersion");
    if (pRtlGetVersion) {
        RTL_OSVERSIONINFOW ovi = { 0 };
        ovi.dwOSVersionInfoSize = sizeof(ovi);
        int ntStatus = pRtlGetVersion(&ovi);
        // On Windows 11 (10.0.22000), CET will require xsave
        if (ntStatus == 0) {
            printf("WHPX on Windows %d.%d.%d detected.\n",
                   ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber);
            if(ovi.dwMajorVersion >= 10 && ovi.dwBuildNumber >= 22000)
                return;
        }
    }

    if (!whpx_has_xsave()) {
        return;
    }

    xsave_cap.AsUINT64 = 0;
    hr = whp_dispatch.WHvSetPartitionProperty(
        whpx->partition,
        WHvPartitionPropertyCodeProcessorXsaveFeatures,
        &xsave_cap,
        sizeof(xsave_cap));

    if (FAILED(hr)) {
        return;
    }

    whpx_xsave_cap.AsUINT64 = 0;
}

static int whpx_accel_init(MachineState *ms)
{
    struct whpx_state *whpx;
    int ret;
    HRESULT hr;
    WHV_CAPABILITY whpx_cap;
    UINT32 whpx_cap_size;
    WHV_PARTITION_PROPERTY prop;

    whpx = &whpx_global;

    if (!init_whp_dispatch()){
        ret = -ENOSYS;
        goto error;
    }

    memset(whpx, 0, sizeof(struct whpx_state));
    memset(whpx_slots, 0, sizeof(whpx_slots));

    whpx->mem_quota = ms->ram_size;

    hr = whp_dispatch.WHvGetCapability(
        WHvCapabilityCodeHypervisorPresent, &whpx_cap,
        sizeof(whpx_cap), &whpx_cap_size);
    if (FAILED(hr) || !whpx_cap.HypervisorPresent) {
        error_report("WHPX: No accelerator found, hr=%08lx", hr);
        ret = -ENOSPC;
        goto error;
    }

    hr = whp_dispatch.WHvCreatePartition(&whpx->partition);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to create partition, hr=%08lx", hr);
        ret = -EINVAL;
        goto error;
    }

    /* Query the XSAVE capability of the partition. */
    hr = whp_dispatch.WHvGetPartitionProperty(
        whpx->partition,
        WHvPartitionPropertyCodeProcessorXsaveFeatures,
        &whpx_xsave_cap,
        sizeof(whpx_xsave_cap),
        &whpx_cap_size);

    /*
     * Windows version which don't support this property will return with the
     * specific error code.
     */
    if (FAILED(hr) && hr != WHV_E_UNKNOWN_PROPERTY) {
        error_report("WHPX: Failed to query XSAVE capability, hr=%08lx", hr);
        ret = -EINVAL;
        goto error;
    }

    /* Disable xsave until the WHPX xsave API's have not been plumbed in */
    whpx_disable_xsave(whpx);

    memset(&prop, 0, sizeof(WHV_PARTITION_PROPERTY));
    prop.ProcessorCount = smp_cpus;
    hr = whp_dispatch.WHvSetPartitionProperty(
        whpx->partition,
        WHvPartitionPropertyCodeProcessorCount,
        &prop,
        sizeof(WHV_PARTITION_PROPERTY));

    if (FAILED(hr)) {
        error_report("WHPX: Failed to set partition core count to %d,"
                     " hr=%08lx", smp_cores, hr);
        ret = -EINVAL;
        goto error;
    }

    memset(&prop, 0, sizeof(WHV_PARTITION_PROPERTY));
    prop.ExtendedVmExits.X64MsrExit = 1;
    prop.ExtendedVmExits.X64CpuidExit = 1;
    hr = whp_dispatch.WHvSetPartitionProperty(
        whpx->partition,
        WHvPartitionPropertyCodeExtendedVmExits,
        &prop,
        sizeof(WHV_PARTITION_PROPERTY));

    if (FAILED(hr)) {
        error_report("WHPX: Failed to enable partition extended X64MsrExit and"
                     " X64CpuidExit hr=%08lx", hr);
        ret = -EINVAL;
        goto error;
    }

    /* We don't know whether HyperV has limitation on how many CPUID leaves
     * can be intercepted. Since the "full" list below can work, let's keep
     * it. Note: the "full" list contains CPUID leaves I can know.
     */
    UINT32 cpuidExitList[] = {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                              0x10, 0x11, 0x12, 0x13,0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                              0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
                              WHPX_CPUID_SIGNATURE,
                              WHPX_CPUID_SIGNATURE + 1,
                              WHPX_CPUID_SIGNATURE + 2,
                              WHPX_CPUID_SIGNATURE + 3,
                              WHPX_CPUID_SIGNATURE + 4,
                              WHPX_CPUID_SIGNATURE + 5,
                              0x80000000,
                              0x80000001,
                              0x80000002,
                              0x80000003,
                              0x80000004,
                              0x80000005,
                              0x80000006,
                              0x80000007,
                              0x80000008,
                              0x80000009,
                              0x8000000a,
                              0x8000000b,
                              0x8000000c,
                              0x8000000d,
                              0x8000000e,
                              0x8000000f,
                              0x80000010,
                              0x80000011,
                              0x80000012,
                              0x80000013,
                              0x80000014,
                              0x80000015,
                              0x80000016,
                              0x80000017,
                              0x80000018,
                              0x80000019,
                              0x8000001a,
                              0x8000001b,
                              0x8000001c,
                              0x8000001d,
                              0x8000001e,
                              0x8000001f,
                              0x80000020};
    hr = whp_dispatch.WHvSetPartitionProperty(
        whpx->partition,
        WHvPartitionPropertyCodeCpuidExitList,
        cpuidExitList,
        RTL_NUMBER_OF(cpuidExitList) * sizeof(UINT32));

    if (FAILED(hr)) {
        error_report("WHPX: Failed to enable partition extended CpuidExitList"
                     " hr=%08lx", hr);
        ret = -EINVAL;
        goto error;
    }

    if (FAILED(hr)) {
        error_report("WHPX: Failed to enable partition extended X64MsrExit and CpuidExit's"
                     " hr=%08lx", hr);
        ret = -EINVAL;
        goto error;
    }

    hr = whp_dispatch.WHvSetupPartition(whpx->partition);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to setup partition, hr=%08lx", hr);
        ret = -EINVAL;
        goto error;
    }

    whpx_memory_init();

    cpu_interrupt_handler = whpx_handle_interrupt;

    printf("Windows Hypervisor Platform accelerator is operational\n");
    return 0;

  error:

    if (NULL != whpx->partition) {
        whp_dispatch.WHvDeletePartition(whpx->partition);
        whpx->partition = NULL;
    }


    return ret;
}

int whpx_enabled(void)
{
    return whpx_allowed;
}

static void whpx_accel_class_init(ObjectClass *oc, void *data)
{
    AccelClass *ac = ACCEL_CLASS(oc);
    ac->name = "WHPX";
    ac->init_machine = whpx_accel_init;
    ac->allowed = &whpx_allowed;
}

static const TypeInfo whpx_accel_type = {
    .name = ACCEL_CLASS_NAME("whpx"),
    .parent = TYPE_ACCEL,
    .class_init = whpx_accel_class_init,
};

static void whpx_type_init(void)
{
    type_register_static(&whpx_accel_type);
}

bool init_whp_dispatch() {
    char* lib_name;
    HMODULE hLib;

    if(whp_dispatch_initialized)
        return true;

    #define WHP_LOAD_FIELD(return_type, function_name, signature) \
        whp_dispatch.function_name = (function_name ## _t)GetProcAddress(hLib, #function_name); \
        if (!whp_dispatch.function_name) { \
            error_report("Could not load function %s from library %s.", \
                         #function_name, lib_name); \
            goto error; \
        } \

    lib_name = "WinHvPlatform.dll";
    hWinHvPlatform = LoadLibrary(lib_name);
    if (!hWinHvPlatform) {
        error_report("Could not load library %s.", lib_name);
        goto error;
    }
    hLib = hWinHvPlatform;
    LIST_WINHVPLATFORM_FUNCTIONS(WHP_LOAD_FIELD)

    lib_name = "WinHvEmulation.dll";
    HMODULE hWinHvEmulation = LoadLibrary(lib_name);
    if (!hWinHvEmulation) {
        error_report("Could not load library %s.", lib_name);
        goto error;
    }
    hLib = hWinHvEmulation;
    LIST_WINHVEMULATION_FUNCTIONS(WHP_LOAD_FIELD)

    whp_dispatch_initialized = true;
    return true;

    error:

    if (hWinHvPlatform)
        FreeLibrary(hWinHvPlatform);
    if (hWinHvEmulation)
        FreeLibrary(hWinHvEmulation);
    return false;
}

type_init(whpx_type_init);
