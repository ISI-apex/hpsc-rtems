#ifndef MEM_MAP_H
#define MEM_MAP_H

// This file indicates the resource allocations in memory

// Binary images with executables for HPPS
#define HPPS_DDR_LOW_ADDR   0x80000000
#define HPPS_DDR_LOW_SIZE   0x3fe00000 // excludes mem hidden from HPPS

// Boot config (TODO: to be replaced by u-boot env/script)
#define HPPS_BOOT_MODE_ADDR 0x9f000000

// Shared memory regions accessible to HPPS
#define HPPS_SHM_ADDR 0xbf800000
#define HPPS_SHM_SIZE   0x400000

// Shared memory regions for userspace
#define HPPS_SHM_ADDR__TRCH_HPPS 0xbf800000
#define HPPS_SHM_SIZE__TRCH_HPPS 0x10000
#define HPPS_SHM_ADDR__HPPS_TRCH 0xbf810000
#define HPPS_SHM_SIZE__HPPS_TRCH 0x10000

// Shared memory regions for SSW
#define HPPS_SHM_ADDR__TRCH_HPPS_SSW 0xbfbf0000
#define HPPS_SHM_SIZE__TRCH_HPPS_SSW 0x08000
#define HPPS_SHM_ADDR__HPPS_TRCH_SSW 0xbfbf8000
#define HPPS_SHM_SIZE__HPPS_TRCH_SSW 0x08000

// Page table should be in memory that is on a bus accessible from the MMUs
// master port ('dma' prop in MMU node in Qemu DT).  We put it in HPPS DRAM,
// because that seems to be the only option, judging from high-level Chiplet
// diagram.
#define RTPS_HPPS_PT_ADDR                0xbfc00000
#define RTPS_HPPS_PT_SIZE                  0x200000

#define RT_MMU_TEST_DATA_LO_ADDR         0xbfe00000
#define RT_MMU_TEST_DATA_LO_SIZE           0x200000

#define WINDOWS_TO_40BIT_ADDR            0xc0000000
#define WINDOWS_TO_40BIT_SIZE            0x20000000

// Within WINDOWS_TO_40BIT_ADDR, windows to RT_MMU_TEST_DATA_HI_x
#define RT_MMU_TEST_DATA_HI_0_WIN_ADDR   0xc0000000
#define RT_MMU_TEST_DATA_HI_1_WIN_ADDR   0xc1000000

// Two regions of same size
#define RT_MMU_TEST_DATA_HI_0_ADDR      0x100000000
#define RT_MMU_TEST_DATA_HI_1_ADDR      0x100010000
#define RT_MMU_TEST_DATA_HI_SIZE            0x10000

// Page tables for RTPS MMU: in RPTS DRAM
#define RTPS_PT_ADDR 0x40010000
#define RTPS_PT_SIZE   0x200000

// DMA Microcode buffer: in RTPS DRAM (cannot be in TCM)
#define RTPS_DMA_MCODE_ADDR 0x40000000
#define RTPS_DMA_MCODE_SIZE 0x00001000

// Buffers for DMA test: in RTPS DRAM
#define RTPS_DMA_SRC_ADDR       0x40001000
#define RTPS_DMA_SIZE           0x00000200
#define RTPS_DMA_DST_ADDR       0x40002000 // align to page
#define RTPS_DMA_DST_REMAP_ADDR 0x40003000 // MMU test maps this to DST_ADDR


#endif // MEM_MAP_H
