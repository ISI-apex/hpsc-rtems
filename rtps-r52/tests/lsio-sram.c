#include <stdint.h>
#include <stdlib.h>

#include <rtems.h>
#include <bsp.h>
#include <bsp/dma.h>
#include <bsp/hwinfo.h>
#include <bsp/sramfs.h>

// plat
#include <mem-map.h>

#include "test.h"

// TODO: test both write and read

int test_lsio_sram(void)
{
    SRAMFS_Config_t sramfs_config;
    uint32_t *load_addr = NULL;
    rtems_status_code sc;
    int rc = 0;

    test_begin("test_lsio_sram");

    sc = sramfs_init(&sramfs_config, SMC_LSIO_SRAM_BASE0, NULL);
    TEST_SC_OR_SET_RC_GOTO(sc, rc, "LSIO_SRAM: init\n", out);

    /* Load a file from "Simple Filesystem" on off-chip memory */
    sc = sramfs_load(&sramfs_config, "test-data", &load_addr);
    TEST_SC_OR_SET_RC_GOTO(sc, rc, "LSIO_SRAM: load 'test-data' file\n", uninit);
    TEST_OR_SET_RC_GOTO(load_addr, rc, "LSIO_SRAM: load_addr: set\n", uninit);

    /* Check content */
    TEST_OR_SET_RC_GOTO(
            (*load_addr & 0xffffffff) == 0x54545454 /* "TEST" in little-endian */,
            rc, "LSIO_SRAM: load_addr: value\n", uninit);

uninit:
    sc = sramfs_uninit(&sramfs_config);
    TEST_SC_OR_SET_RC(sc, rc, "LSIO_SRAM: uninit\n");

out:
    test_end("test_lsio_sram", rc);
    return rc;
}

int test_lsio_sram_dma(void)
{
    DMA_Config_t dma_config;
    SRAMFS_Config_t sramfs_config;
    uint32_t *load_addr = (uint32_t *)RTPS_DDR_ADDR__RTPS_R52_LOCKSTEP;
    rtems_status_code sc;
    int rc = 0;

    test_begin("test_lsio_sram_dma");

    /* Initialize DMA controller */
    sc = dma_init(&dma_config, BSP_DMA_BASE);
    TEST_SC_OR_SET_RC_GOTO(sc, rc, "LSIO_SRAM_DMA: dma_init\n", out);

    sc = sramfs_init(&sramfs_config, SMC_LSIO_SRAM_BASE0, &dma_config);
    TEST_SC_OR_SET_RC_GOTO(sc, rc, "LSIO_SRAM_DMA: sramfs_init\n", uninit_dma);

    /* Load a file from "Simple Filesystem" on off-chip memory */
    sc = sramfs_load_addr(&sramfs_config, "test-data", load_addr);
    TEST_SC_OR_SET_RC_GOTO(sc, rc, "LSIO_SRAM_DMA: load 'test-data' file\n", uninit);

    /* Check content */
    TEST_OR_SET_RC_GOTO(
            (*load_addr & 0xffffffff) == 0x54545454 /* "TEST" in little-endian */,
            rc, "LSIO_SRAM_DMA: load_addr: value\n", uninit);

uninit:
    sc = sramfs_uninit(&sramfs_config);
    TEST_SC_OR_SET_RC(sc, rc, "LSIO_SRAM_DMA: sramfs_uninit\n");

uninit_dma:
    sc = dma_uninit(&dma_config);
    TEST_SC_OR_SET_RC(sc, rc, "LSIO_SRAM_DMA: dma_uninit\n");

out:
    test_end("test_lsio_sram_dma", rc);
    return rc;
}
