#include <stdbool.h>
#include <stdio.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <bsp.h>
#include <bsp/mmu_500.h>

// plat
#include <hwinfo.h>
#include <mem-map.h>

#include "test.h"

#define RTPS_DMA_SIZE_12BIT     0x00001000

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

int test_rtps_mmu(bool do_dma_test)
{
    MMU_Config_t mmu_config;
    MMU_Context_t *mmu_context;
    MMU_Stream_t *mmu_stream;
    rtems_id region_id;
    rtems_status_code sc;
    int rc = 0;

    test_begin("test_rtps_mmu");

    sc = mmu_init(&mmu_config, RTPS_SMMU_BASE);
    SC_CHECK(sc, rc, "MMU: init\n", out);

    sc = mmu_disable(&mmu_config);
    SC_CHECK(sc, rc, "MMU: disable\n", out_uninit);

    sc = rtems_region_create(rtems_build_name('M', 'M', 'U', 'T'),
                             (void *)RTPS_PT_ADDR,
                             RTPS_PT_SIZE, 0x1000 /* 4KB page size */,
                             RTEMS_PRIORITY, &region_id);
    SC_CHECK(sc, rc, "MMU: region create\n", out_uninit);

    sc = mmu_context_create(&mmu_context, &mmu_config, region_id,
                            MMU_PAGESIZE_4KB);
    SC_CHECK(sc, rc, "MMU: context create\n", out_uninit);

    sc = mmu_stream_create(&mmu_stream, MASTER_ID_RTPS_DMA, mmu_context);
    SC_CHECK(sc, rc, "MMU: stream create\n", out_context_destroy);

    /*
     * MMU maps RTPS_DMA_DST_REMAP_ADDR to RTPS_DMA_DST_ADDR and passthrough for
     * other required memory regions
     */
    sc = mmu_map(mmu_context,
                 (void *)RTPS_DMA_MCODE_ADDR, (void *)RTPS_DMA_MCODE_ADDR,
                 RTPS_DMA_MCODE_SIZE);
    SC_CHECK(sc, rc, "MMU: map 1\n", out_stream_destroy);
    sc = mmu_map(mmu_context,
                 (void *)RTPS_DMA_SRC_ADDR, (void *)RTPS_DMA_SRC_ADDR,
                 RTPS_DMA_SIZE_12BIT);
    SC_CHECK(sc, rc, "MMU: map 2\n", out_unmap1);
    sc = mmu_map(mmu_context,
                 (void *)RTPS_DMA_DST_ADDR, (void *)RTPS_DMA_DST_ADDR,
                 RTPS_DMA_SIZE_12BIT);
    SC_CHECK(sc, rc, "MMU: map 3\n", out_unmap2);
    sc = mmu_map(mmu_context,
                 (void *)RTPS_DMA_DST_REMAP_ADDR, (void *)RTPS_DMA_DST_ADDR,
                 RTPS_DMA_SIZE_12BIT);
    SC_CHECK(sc, rc, "MMU: map 4\n", out_unmap3);

    sc = mmu_enable(&mmu_config);
    SC_CHECK(sc, rc, "MMU: enable\n", out_unmap4);

    if (do_dma_test) {
        rc = test_rtps_dma();
    }

out_unmap4:
    sc = mmu_unmap(mmu_context, (void *)RTPS_DMA_DST_REMAP_ADDR,
                   RTPS_DMA_SIZE_12BIT);
    SC_RC(sc, rc, "MMU: unmap 4\n");
out_unmap3:
    sc = mmu_unmap(mmu_context, (void *)RTPS_DMA_DST_ADDR, RTPS_DMA_SIZE_12BIT);
    SC_RC(sc, rc, "MMU: unmap 3\n");
out_unmap2:
    sc = mmu_unmap(mmu_context, (void *)RTPS_DMA_SRC_ADDR, RTPS_DMA_SIZE_12BIT);
    SC_RC(sc, rc, "MMU: unmap 2\n");
out_unmap1:
    sc = mmu_unmap(mmu_context, (void *)RTPS_DMA_MCODE_ADDR,
                   RTPS_DMA_MCODE_SIZE);
    SC_RC(sc, rc, "MMU: unmap 1\n");

out_stream_destroy:
    sc = mmu_stream_destroy(mmu_stream);
    SC_RC(sc, rc, "MMU: stream destroy\n");

out_context_destroy:
    sc = mmu_context_destroy(mmu_context);
    SC_RC(sc, rc, "MMU: context destroy\n");

out_uninit:
    sc = mmu_uninit(&mmu_config);
    SC_RC(sc, rc, "MMU: uninit\n");

out:
    test_end("test_rtps_mmu", rc);
    return rc;
}
