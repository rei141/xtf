#pragma once
#include <xtf.h>

#define APIC_VERSION   0x30
#define APIC_TPR       0x80
#define APIC_EOI       0xB0
#define APIC_SVR       0xF0
#define APIC_ICR_LOW   0x300
#define APIC_ICR_HIGH  0x310

#define APIC_ENABLE    0x100
#define APIC_FOCUS_DISABLE (1 << 9)

typedef void (*FuncTable)(void);

#define get64b(x) ((uint64_t *)(input_buf + x))[0]
#define get32b(x) ((uint32_t *)(input_buf + x))[0]
#define get16b(x) ((uint16_t *)(input_buf + x))[0]
#define get8b(x) ((uint8_t *)(input_buf + x))[0]

// #define get64b(x) (((uint64_t)genrand_int32() << 32) | genrand_int32())
// #define get32b(x) (uint32_t)genrand_int32()
// #define get16b(x) (uint16_t)(genrand_int32() & 0xFFFF)
// #define get8b(x) (uint8_t)(genrand_int32() & 0xFF)

#define write64b(x, v) ((uint64_t *)(input_buf + x))[0] = (uint64_t)v
#define write32b(x, v) ((uint32_t *)(input_buf + x))[0] = (uint32_t)v
#define write16b(x, v) ((uint16_t *)(input_buf + x))[0] = (uint16_t)v
#define write8b(x, v) ((uint8_t *)(input_buf + x))[0] = (uint8_t)v

extern uint8_t *input_buf;
extern uint64_t index_selector_count;
extern volatile uint64_t *apic_base;
extern FuncTable exec_l1_table[];
extern FuncTable exec_l2_table[];
extern const size_t L1_TABLE_SIZE;
extern const size_t L2_TABLE_SIZE;
void exec_cpuid(void);
void exec_hlt(void);
void exec_invd(void);
void exec_invlpg(void);
void exec_rdpmc(void);
void exec_rdtsc(void);
void exec_rsm(void);
void exec_vmclear(void);
void exec_vmlaunch(void);
void exec_l1_vmptrst(void);
void exec_l2_vmptrst(void);
void exec_vmptrld(void);

void exec_l1_vmread(void);
void exec_l1_vmwrite(void);
void exec_l2_vmread(void);
void exec_l2_vmwrite(void);
void exec_vmxoff(void);
void exec_vmxon(void);
void exec_vmresue(void);

void exec_cr(void);
void exec_dr(void);
void exec_io(void);

void exec_rdmsr(void);
void exec_wrmsr(void);
void exec_mwait(void);
void exec_monitor(void);
void exec_pause(void);
void exec_rdtscp(void);
void exec_invept(void);
void exec_invvpid(void);
void exec_wb(void);
void exec_xset(void);
void exec_rdrand(void);
void exec_invpcid(void);
void exec_vmfunc(void);
void exec_encls(void);
void exec_rdseed(void);

void exec_pconfig(void);
void exec_msr_save_load(void);
void exec_page_table(void);

uint32_t read_local_apic_id(void);
uint32_t read_local_apic_version(void);
void write_eoi(void);
void write_icr(void);
void read_icr(void);
void exec_apic(void);
void __invpcid(unsigned long pcid, unsigned long addr,
                             unsigned long type);