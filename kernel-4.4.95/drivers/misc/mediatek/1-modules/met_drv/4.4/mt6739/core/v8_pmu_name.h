
#ifndef _V8_PMU_NAME_H_
#define _V8_PMU_NAME_H_

/* Cortex-A53 */
struct pmu_desc a53_pmu_desc[] = {
	{0x00, "SW_INCR"},
	{0x01, "L1I_CACHE_REFILL"},
	{0x02, "L1I_TLB_REFILL"},
	{0x03, "L1D_CACHE_REFILL"},
	{0x04, "L1D_CACHE"},
	{0x05, "L1D_TLB_REFILL"},
	{0x06, "LD_RETIRED"},
	{0x07, "ST_RETIRED"},
	{0x08, "INST_RETIRED"},
	{0x09, "EXC_TAKEN"},
	{0x0A, "EXC_RETURN"},
	{0x0B, "CID_WRITE_RETIRED"},
	{0x0C, "PC_WRITE_RETIRED"},
	{0x0D, "BR_IMMED_RETIRED"},
	{0x0E, "BR_RETURN_RETIRED"},
	{0x0F, "UNALIGNED_LDST_RETIRED"},
	{0x10, "BR_MIS_PRED"},
	{0x11, "CPU_CYCLES"},
	{0x12, "BR_PRED"},
	{0x13, "MEM_ACCESS"},
	{0x14, "L1I_CACHE"},
	{0x15, "L1D_CACHE_WB"},
	{0x16, "L2D_CACHE"},
	{0x17, "L2D_CACHE_REFILL"},
	{0x18, "L2D_CACHE_WB"},
	{0x19, "BUS_ACCESS"},
	{0x1A, "MEMORY_ERROR"},
	{0x1D, "BUS_CYCLES"},
	{0x60, "BUS_READ_ACCESS"},
	{0x61, "BUS_WRITE_ACCESS"},
	{0x86, "IRQ_EXC_TAKEN"},
	{0x87, "FIQ_EXC_TAKEN"},
	{0xC0, "EXT_MEM_REQ"},
	{0xC1, "NO_CACHE_EXT_MEM_REQ"},
	{0xC2, "PREFETCH_LINEFILL"},
	{0xC4, "ENT_READ_ALLOC_MODE"},
	{0xC5, "READ_ALLOC_MODE"},
	{0xC6, "PRE_DECODE_ERROR"},
	{0xC7, "WRITE_STALL"},
	{0xC8, "SCU_SNOOP_DATA_FROM_ANOTHER_CPU"},
	{0xC9, "CONDITIONAL_BRANCH_EXE"},
	{0xCA, "INDIRECT_BRANCH_MISPREDICT"},
	{0xCB, "INDIRECT_BRANCH_MISPREDICT_ADDR"},/*"INDIRECT_BRANCH_MISPREDICT_ADDR_MISSCOMPARE" */
	{0xCC, "COND_BRANCH_MISPREDICT"},
	{0xD0, "L1_INST_CACHE_MEM_ERR"},
	{0xE1, "ICACHE_MISS_STALL"},
	{0xE2, "DPU_IQ_EMPTY"},
	{0xE4, "NOT_FPU_NEON_INTERLOCK"},
	{0xE5, "LOAD_STORE_INTERLOCK"},
	{0xE6, "FPU_NEON_INTERLOCK"},
	{0xE7, "LOAD_MISS_STALL"},
	{0xE8, "STORE_STALL"},
	{0xFF, "CPU_CYCLES"}
};

#define A53_PMU_DESC_COUNT (sizeof(a53_pmu_desc) / sizeof(struct pmu_desc))

#endif				/* _V8_PMU_NAME_H_ */

