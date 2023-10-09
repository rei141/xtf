#include "fuzz.h"

#include "msr.h"
#include "vmx.h"


extern char vmxon_region[4096] __attribute__ ((aligned (4096)));

FuncTable exec_l1_table[] = {
    exec_cpuid,      exec_hlt,        exec_invd,       exec_invlpg,
    exec_rdpmc,      exec_rdtsc,      exec_rsm,        exec_vmclear,
    exec_vmlaunch,   exec_vmptrld,    exec_l1_vmptrst, exec_l1_vmread,
    exec_vmresue,    exec_vmxoff,     exec_vmxon,      exec_cr,
    exec_dr,         exec_io,         exec_rdmsr,      exec_wrmsr,
    exec_mwait,      exec_monitor,    exec_pause,      exec_invept,
    exec_rdtscp,     exec_invvpid,    exec_wb,         exec_xset,
    exec_rdrand,     exec_invpcid,    exec_vmfunc,     exec_encls,
    exec_rdseed,     exec_pconfig,    exec_l2_vmptrst, exec_l2_vmread,
    exec_l1_vmwrite, exec_l2_vmwrite, exec_page_table, exec_msr_save_load,exec_apic

};
FuncTable exec_l2_table[] = {
    exec_cpuid,      exec_hlt,     exec_invd,       exec_invlpg,
    exec_rdpmc,      exec_rdtsc,   exec_rsm,        exec_vmclear,
    exec_vmlaunch,   exec_vmptrld, exec_l2_vmptrst, exec_l2_vmread,
    exec_vmresue,    exec_vmxoff,  exec_vmxon,      exec_cr,
    exec_dr,         exec_io,      exec_rdmsr,      exec_wrmsr,
    exec_mwait,      exec_monitor, exec_pause,      exec_invept,
    exec_rdtscp,     exec_invvpid, exec_wb,         exec_xset,
    exec_rdrand,     exec_invpcid, exec_vmfunc,     exec_encls,
    exec_rdseed,     exec_pconfig, exec_l2_vmwrite, exec_msr_save_load,
    exec_page_table,exec_apic
};

const size_t L1_TABLE_SIZE = sizeof(exec_l1_table) / sizeof(FuncTable);
const size_t L2_TABLE_SIZE = sizeof(exec_l2_table) / sizeof(FuncTable);
extern int vmcs_num;
extern uint16_t vmcs_index[];
void exec_cpuid() {
    if (get8b(index_selector_count++) % 3 == 0) {
        asm volatile("cpuid" ::"a"(get8b(index_selector_count++) % 0x21),
                     "c"(get8b(index_selector_count++) % 0x21)
                     : "ebx", "edx");
    }
    if (get8b(index_selector_count++) % 3 == 1) {
        asm volatile("cpuid" ::"a"(0x80000000 | get8b(index_selector_count++) % 0x9)
                     : "ebx", "edx");
    } else {
        asm volatile("cpuid" ::"a"(0x4fffffff & (get32b(index_selector_count++)))
                     : "ebx", "edx");
        index_selector_count += 4;
    }
}

void exec_hlt() { asm volatile("hlt"); }

void exec_invd() {
    asm volatile("invd");  // 13
}

void exec_invlpg() {
    uint64_t p;
    p = get64b(index_selector_count);
    index_selector_count += 8;
    asm volatile("invlpg %0" : : "m"(p));  // 14 vmexit o
}
void exec_rdpmc() {
    uint64_t p;
    p = get64b(index_selector_count);
    index_selector_count += 8;
    asm volatile("rdpmc" : "+c"(p) : : "%rax");  // 15 vmexit o sometimes hang
}
void exec_rdtsc() {
    asm volatile("rdtsc");  // 16
}
void exec_rsm() {
    asm volatile("rsm");  // 16
}
void exec_vmclear() {
    uint64_t value = get64b(index_selector_count);
    index_selector_count += 8;

    if (get8b(index_selector_count++) % 2) {
        asm volatile("vmclear %0" ::"m"(vmxon_region));
    } else {
        asm volatile("vmclear %0" ::"m"(value));
    }
}
void exec_vmlaunch() { asm volatile("vmlaunch\n\t"); }
void exec_l1_vmptrst() {
    uint64_t value;
    vmptrst(&value);
}
void exec_l2_vmptrst() {
    uint64_t value;
    asm volatile("vmptrst %0" : : "m"(value) : "cc");
}

void exec_vmptrld() {
    uint64_t value = get64b(index_selector_count);
    index_selector_count += 8;
    vmptrld(&value);
}

void exec_l1_vmread() {
    vmread(vmcs_index[get16b(index_selector_count) % vmcs_num]);
    index_selector_count += 2;
}
void exec_l1_vmwrite() {
    uint64_t value = get64b(index_selector_count);
    index_selector_count += 8;
    vmwrite(vmcs_index[get16b(index_selector_count) % vmcs_num], value);
    index_selector_count += 2;
}
void exec_l2_vmread() {
    if (get8b(index_selector_count++) % 2) {
        uint64_t *v = (uint64_t *)get64b(index_selector_count);
        index_selector_count += 8;
        asm volatile(
            "vmread %1, %0"
            : "=m"(v)
            : "a"((uint64_t)(vmcs_index[get16b(index_selector_count) % vmcs_num]))
            : "cc");
        index_selector_count += 2;
    } else {
        uint64_t value;
        asm volatile("vmread %%rax, %%rdx"
                     : "=d"(value)
                     : "a"(vmcs_index[get16b(index_selector_count) % vmcs_num])
                     : "cc");
        index_selector_count += 2;
    }
}
void exec_l2_vmwrite() {
    uint64_t value = get64b(index_selector_count);
    index_selector_count += 8;
    if (get8b(index_selector_count++) % 2) {
        uint64_t *v = (uint64_t *)value;
        asm volatile(
            "vmwrite %1, %0"
            :
            : "a"((uint64_t)(vmcs_index[get16b(index_selector_count) % vmcs_num])),
              "m"(v)
            : "cc");
        index_selector_count += 2;
    } else {
        asm volatile("vmwrite %%rdx, %%rax"
                     :
                     : "a"(vmcs_index[get16b(index_selector_count) % vmcs_num]),
                       "d"(value)
                     : "cc", "memory");
        index_selector_count += 2;
    }
}
void exec_vmxoff() { asm volatile("vmxoff"); }

void exec_vmxon() {
    uint64_t value = get64b(index_selector_count);
    index_selector_count += 8;
    asm volatile("vmxon %0" ::"m"(value));
}
void exec_vmresue() {
    // asm volatile("vmresume\n\t");
}

void exec_cr() {
    uint64_t value, zero;
    switch (get8b(index_selector_count++) % 4) {
        case 0:
            value = get64b(index_selector_count);
            index_selector_count += 8;
            switch (get8b(index_selector_count++) % 4) {
                case 0:
                    asm volatile("movq %0, %%cr0" : "+c"(value) : : "%rax");
                    break;
                case 1:
                    asm volatile("movq %0, %%cr3" : "+c"(value) : : "%rax");
                    break;
                case 2:
                    asm volatile("movq %0, %%cr4" : "+c"(value) : : "%rax");
                    break;
                case 3:
                    asm volatile("movq %0, %%cr8" : "+c"(value) : : "%rax");
                    break;
            }
            break;
        case 1:
            switch (get8b(index_selector_count++) % 4) {
                case 0:
                    asm volatile("movq %%cr0, %0" : "=c"(zero) : : "%rbx");
                    break;
                case 1:
                    asm volatile("movq %%cr3, %0" : "=c"(zero) : : "%rbx");
                    break;
                case 2:
                    asm volatile("movq %%cr4, %0" : "=c"(zero) : : "%rbx");
                    break;
                case 3:
                    asm volatile("movq %%cr8, %0" : "=c"(zero) : : "%rbx");
                    break;
            }
            break;
        case 2:
            asm volatile("clts");
            break;
        case 3:
            value = get16b(index_selector_count);
            index_selector_count += 2;
            asm volatile("lmsw %0" : : "m"(value));
            break;
    }
}

void exec_dr() {
    uint64_t zero;
    asm volatile("movq %%dr0, %0" : "=c"(zero) : : "%rbx");
    asm volatile("movq %%dr1, %0" : "=c"(zero) : : "%rbx");
    asm volatile("movq %%dr2, %0" : "=c"(zero) : : "%rbx");
    asm volatile("movq %%dr3, %0" : "=c"(zero) : : "%rbx");
    asm volatile("movq %%dr4, %0" : "=c"(zero) : : "%rbx");
    asm volatile("movq %%dr5, %0" : "=c"(zero) : : "%rbx");
    asm volatile("movq %%dr6, %0" : "=c"(zero) : : "%rbx");
    asm volatile("movq %%dr7, %0" : "=c"(zero) : : "%rbx");
    asm volatile("movq %0, %%dr0" : "+c"(get64b(index_selector_count)) : : "%rax");
    index_selector_count += 8;
    asm volatile("movq %0, %%dr1" : "+c"(get64b(index_selector_count)) : : "%rax");
    index_selector_count += 8;
    asm volatile("movq %0, %%dr2" : "+c"(get64b(index_selector_count)) : : "%rax");
    index_selector_count += 8;
    asm volatile("movq %0, %%dr3" : "+c"(get64b(index_selector_count)) : : "%rax");
    index_selector_count += 8;
    asm volatile("movq %0, %%dr4" : "+c"(get64b(index_selector_count)) : : "%rax");
    index_selector_count += 8;
    asm volatile("movq %0, %%dr5" : "+c"(get64b(index_selector_count)) : : "%rax");
    index_selector_count += 8;
    asm volatile("movq %0, %%dr6" : "+c"(get64b(index_selector_count)) : : "%rax");
    index_selector_count += 8;
    asm volatile("movq %0, %%dr7" : "+c"(get64b(index_selector_count)) : : "%rax");
    index_selector_count += 8;
}

void exec_io() {
    if (get8b(index_selector_count++) % 2) {
        asm volatile("mov %0, %%dx" ::"r"(get16b(index_selector_count)));
        index_selector_count += 2;
        asm volatile("mov %0, %%eax" ::"r"(get32b(index_selector_count)));
        asm volatile("out %eax, %dx");
        index_selector_count += 4;
    } else {
        asm volatile("mov %0, %%dx" ::"r"(get16b(index_selector_count)));
        index_selector_count += 2;
        asm volatile("in %dx, %eax");
    }
}

void exec_rdmsr() {
    uint32_t index = msr_table[get16b(index_selector_count) % MSR_TABLE_SIZE];
    index_selector_count += 2;
    if (get8b(index_selector_count++) % 2) {
        asm volatile("rdmsr" ::"c"(index));
    } else {
        index = get32b(index_selector_count);
        index_selector_count += 4;
        asm volatile("rdmsr" ::"c"(index));
    }
}
void exec_wrmsr() {
    uint32_t index = msr_table[get16b(index_selector_count) % MSR_TABLE_SIZE];
    index_selector_count += 2;
    uint64_t value = get64b(index_selector_count);
    index_selector_count += 8;

    if (get8b(index_selector_count++) % 2) {
        asm volatile("wrmsr" ::"c"(index), "a"(value & 0xFFFFFFFF),
                     "d"(value >> 32));
    } else {
        index = get32b(index_selector_count);
        index_selector_count += 4;
        asm volatile("wrmsr" ::"c"(index), "a"(value & 0xFFFFFFFF),
                     "d"(value >> 32));
        // asm volatile("wrmsr" ::"c"(0xC0000000 | (index & 0x1FFF)), "a"(value
        // & 0xFFFFFFFF), "d"(value >> 32));
    }
}
void exec_mwait() {
    asm volatile("mwait");  // 36
}
void exec_monitor() {
    asm volatile("monitor");  // 39
}
void exec_pause() {
    asm volatile("pause");  // 40
}
void exec_rdtscp() {
    asm volatile("rdtscp");  // 51 vmexit sometimes hang
}
void exec_invept() {
    invept_t inv;
    inv.rsvd = 0;
    inv.ptr = get64b(index_selector_count);
    index_selector_count += 8;
    int type = get8b(index_selector_count++) % 4;
    invept((uint64_t)type, &inv);
}
void exec_invvpid() {
    invvpid_t inv;
    inv.rsvd = 0;
    inv.gva = get64b(index_selector_count);
    index_selector_count += 8;
    inv.vpid = get16b(index_selector_count);
    index_selector_count += 2;
    int type = get8b(index_selector_count++) % 4;
    invvpid((uint64_t)type, &inv);
}

void exec_wb() {
    if (get8b(index_selector_count++) % 2) {
        asm volatile("wbnoinvd" :::);  // 54
    } else {
        asm volatile("wbinvd" :::);  // 54
    }
}

void exec_xset() {
    asm volatile("xsetbv" :::);  // 55 sometimes hang
}

void exec_rdrand() {
    uint64_t zero = 0;
    asm volatile("rdrand %0" : "+c"(zero) : : "%rax");  // 57
}
void exec_invpcid() {
    __invpcid(0, 0, 0);  // 58 vmexit sometimes hang
}

void exec_vmfunc() {
    uint64_t value = get16b(index_selector_count++) % 512;
    index_selector_count += 2;
    asm volatile("mov %0, %%rcx" ::"d"(value) :);
    // asm volatile ("mov 0, %eax");
    // asm volatile ("vmfunc":::);
    asm volatile("mov 0, %eax");
    asm volatile("vmfunc" :::);
}

void exec_encls() {
    asm volatile("encls" :::);  // 60 vmexit sometimes hang
}

void exec_rdseed() {
    uint64_t zero = 0;
    asm volatile("rdseed %0" : "+c"(zero) : : "%rax");  // 61
}

void exec_pconfig() {
    asm volatile("pconfig");  // 65 vmexit sometimes hang
}
extern uint64_t msr_load[1024] __attribute__((aligned(4096)));
extern uint64_t msr_store[1024] __attribute__((aligned(4096)));
extern uint64_t vmentry_msr_load[1024] __attribute__((aligned(4096)));
void exec_msr_save_load() {
    int i = get16b(index_selector_count++) % 512;
    index_selector_count += 2;
    int selector = get8b(index_selector_count++) % 3;
    uint32_t index = msr_table[get16b(index_selector_count++) % MSR_TABLE_SIZE];
    index_selector_count += 2;
    uint64_t value = get64b(index_selector_count);
    index_selector_count += 8;
    switch (selector) {
        case 0:
            msr_store[i * 2] = index;
            msr_store[i * 2 + 1] = value;
            break;
        case 1:
            msr_load[i * 2] = index;
            msr_load[i * 2 + 1] = value;
            break;
        case 2:
            vmentry_msr_load[i * 2] = index;
            vmentry_msr_load[i * 2 + 1] = value;
            break;
        default:
            break;
    }
}
extern const uint64_t kPageSize4K;
extern const uint64_t kPageSize2M;
extern const uint64_t kPageSize1G;

extern uint64_t pml4_table[512] __attribute__((aligned(4096)));
extern uint64_t pdp_table[512] __attribute__((aligned(4096)));
extern uint64_t page_directory[512][512] __attribute__((aligned(4096)));
extern uint64_t pml4_table_2[512] __attribute__((aligned(4096)));
void exec_page_table() {
    uint8_t ept_xwr = get8b(index_selector_count++) & 0xf;
    uint16_t ept_mode = get16b(index_selector_count++) & 0xff0;
    index_selector_count += 2;
    pml4_table[0] = (uint64_t)&pdp_table[0] | ept_mode | ept_xwr;
    pml4_table_2[0] = (uint64_t)&pdp_table[0] | ept_mode | ept_xwr;

    uint32_t i_pdpt = get16b(index_selector_count++) % 512;
    index_selector_count += 2;
    uint32_t i_pd = get16b(index_selector_count++) % 512;
    index_selector_count += 2;

    pdp_table[i_pdpt] = (uint64_t)&page_directory[i_pdpt] | ept_mode | ept_xwr;

    page_directory[i_pdpt][i_pd] =
        (i_pdpt * kPageSize1G + i_pd * kPageSize2M) | ept_mode | ept_xwr;
    // for (int i_pdpt = 0; i_pdpt < 512; ++i_pdpt)
    // {
    //     pdp_table[i_pdpt] = (uint64_t)&page_directory[i_pdpt] | ept_mode |
    //     ept_xwr; for (int i_pd = 0; i_pd < 512; ++i_pd)
    //     {
    //         page_directory[i_pdpt][i_pd] = (i_pdpt * kPageSize1G + i_pd *
    //         kPageSize2M) | ept_mode | 0;
    //     }
    // }
    // wprintf(L" ept 0x%x\n", ept_mode|ept_xwr);
}


uint32_t read_local_apic_id() {
    volatile uint32_t *local_apic_id = (uint32_t *)(apic_base + APIC_ID);
    uint32_t value = *local_apic_id;
    return value;
}

uint32_t read_local_apic_version() {
    volatile uint32_t *local_apic_version =
        (uint32_t *)(apic_base + APIC_VERSION);
    uint32_t value = *local_apic_version;
    return value;
}

void write_eoi() {
    volatile uint32_t *eoi_register = (uint32_t *)(apic_base + APIC_EOI);
    *eoi_register = 0;
}

void write_icr() {
    uint64_t value = get64b(index_selector_count);
    index_selector_count += 8;
    volatile uint32_t *icr_low = (uint32_t *)(apic_base + APIC_ICR_LOW);
    volatile uint32_t *icr_high = (uint32_t *)(apic_base + APIC_ICR_HIGH);
    *icr_low = value & 0xFFFFFFFF;
    *icr_high = value >> 32;
}

void read_icr() {
    volatile uint32_t *icr_low = (uint32_t *)(apic_base + APIC_ICR_LOW);
    volatile uint32_t *icr_high = (uint32_t *)(apic_base + APIC_ICR_HIGH);
    uint64_t value = ((uint64_t)(*icr_high) << 32) | *icr_low;
    value++;
}

void exec_apic() {
    uint8_t command = input_buf[index_selector_count++];

    switch (command % 5) {
        case 0:
            read_local_apic_id();
            break;
        case 1:
            read_local_apic_version();
            break;
        case 2:
            write_eoi();
            break;
        case 3:
            write_icr();
            break;
        case 4:
            read_icr();
            break;
    }
}

void __invpcid(unsigned long pcid, unsigned long addr,
                             unsigned long type) {
    struct {
        uint64_t d[2];
    } desc = {{pcid, addr}};
    /*
     * The memory clobber is because the whole point is to invalidate
     * stale TLB entries and, especially if we're flushing global
     * mappings, we don't want the compiler to reorder any subsequent
     * memory accesses before the TLB flush.
     *
     * The hex opcode is invpcid (%ecx), %eax in 32-bit mode and
     * invpcid (%rcx), %rax in long mode.
     */
    asm volatile(".byte 0x66, 0x0f, 0x38, 0x82, 0x01"
                 :
                 : "m"(desc), "a"(type), "c"(&desc)
                 : "memory");
}