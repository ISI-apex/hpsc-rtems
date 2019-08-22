#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <bsp.h>
#include <bsp/dma.h>
#include <bsp/dma_330.h>

// plat
#include <mem-map.h>

#include "test.h"

#define SC_RC(s, r, msg) \
    if (s != RTEMS_SUCCESSFUL) { \
        r = 1; \
        printk(msg); \
    }

#define SC_CHECK(s, r, msg, lbl) \
    if (s != RTEMS_SUCCESSFUL) { \
        r = 1; \
        printk(msg); \
        goto lbl; \
    }

int test_rtps_dma(void)
{
    DMA_Config_t dma_config;
    DMA_Channel_t *dma_channel = (DMA_Channel_t*)RTPS_DMA_MCODE_ADDR;
    uint32_t *src_buf = (uint32_t *)RTPS_DMA_SRC_ADDR;
    uint32_t *dst_buf = (uint32_t *)RTPS_DMA_DST_ADDR;
    size_t i;
    bool stopped = false;
    bool faulted = false;
    bool transferred;
    rtems_status_code sc;
    int rc = 0;

    test_begin("test_dma");

    /* Verify DMA controller supports any channels at all */
    if (BSP_DMA_MAX_CHANNELS == 0) {
        printk("BSP_DMA_MAX_CHANNELS Check FAILED -- expected (>0) got (0)\n");
        rc = 1;
        goto out;
    }

    sc = dma_init(&dma_config, BSP_DMA_BASE);
    SC_CHECK(sc, rc, "DMA: init\n", out);

    sc = dma_channel_alloc(&dma_config, dma_channel, 0);
    SC_CHECK(sc, rc, "DMA: channel alloc\n", out_uninit);

    for (i = 0; i < (RTPS_DMA_SIZE / sizeof(uint32_t)); i++) {
        src_buf[i] = 0xf00d0000 | i;
    }

    sc = dma_copy_memory_to_memory(dma_channel,
                                   (uint32_t *)RTPS_DMA_DST_REMAP_ADDR,
                                   src_buf, RTPS_DMA_SIZE);
    SC_CHECK(sc, rc, "DMA: copy\n", out_channel_free);

    /* Wait for the channel to finish executing */
    while (!stopped && !faulted) {
        /* Get the current execution sc of the DMA channel */
        sc = dma_channel_is_stopped(dma_channel, &stopped);
        SC_CHECK(sc, rc, "DMA: wait: stopped\n", out_channel_free);
        sc = dma_channel_is_faulted(dma_channel, &faulted);
        SC_CHECK(sc, rc, "DMA: wait: faulted\n", out_channel_free);
    }

    /* Check whether there was a fault during program execution */
    if (faulted) {
        printk("DMA: execution fault!\n");
        rc = 1;
        goto out_channel_free;
    }

    /* Check that the virtual access via mapped memory was successful */
    transferred = memcmp(src_buf, dst_buf, RTPS_DMA_SIZE);
    if (transferred) {
        printk("DMA: memory transfer failed!\n");
        rc = 1;
        goto out_channel_free;
    }

out_channel_free:
    sc = dma_channel_free(dma_channel);
    SC_RC(sc, rc, "DMA: channel free\n");

out_uninit:
    sc = dma_uninit(&dma_config);
    SC_RC(sc, rc, "DMA: uninit\n");

out:
    test_end("test_dma", rc);
    return rc;
}
