#pragma once
#include <xtf.h>

extern uint32_t msr_table[];
extern const size_t MSR_TABLE_SIZE;

#define MSR_IA32_APICBASE 0x0000001b
#define MSR_IA32_APICBASE_BSP (1 << 8)
#define MSR_IA32_APICBASE_ENABLE (1 << 11)
#define MSR_IA32_APICBASE_BASE (0xfffff << 12)
