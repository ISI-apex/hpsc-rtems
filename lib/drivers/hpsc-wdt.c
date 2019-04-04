#define DEBUG 0

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <rtems.h>
#include <rtems/bspIo.h>
#include <bsp/fatal.h>
#include <bsp/irq-generic.h>

#include "bit.h"
#include "debug.h"
#include "hpsc-wdt.h"
#include "regops.h"

// Stage-relative offsets
#define REG__TERMINAL        0x00
#define REG__COUNT           0x08

#define NUM_STAGES           2
#define STAGE_REG_SIZE       8

// Global offsets
#define REG__CONFIG          0x20
#define REG__STATUS          0x24

#define REG__CMD_ARM         0x28
#define REG__CMD_FIRE        0x2c

// Fields
#define REG__CONFIG__EN      0x1
#define REG__CONFIG__TICKDIV__SHIFT 2
#define REG__STATUS__TIMEOUT__SHIFT 0

#define STAGE_REGS_SIZE (NUM_STAGES * STAGE_REG_SIZE) // register space size per stage
#define STAGE_REG(reg, stage) ((STAGE_REGS_SIZE * stage) + reg)

// The split by command type is for driver implementation convenience, in
// hardware, there is only one command type.

enum cmd {
    CMD_DISABLE = 0,
    NUM_CMDS,
};

enum stage_cmd {
    SCMD_CAPTURE = 0,
    SCMD_LOAD,
    SCMD_CLEAR,
    NUM_SCMDS,
};

struct cmd_code {
    uint32_t arm;
    uint32_t fire;
};

// Store the codes in RO memory section. From fault tolerance perspective,
// there is not a big difference with storing them in instruction memory (i.e.
// by using them as immediate values) -- and the compiler is actually free to
// do so. Both are protected by the MPU, and an upset in both will be detected
// by ECC. In both cases, the value can end up in the cache (either I or D,
// both of which have ECC). Also, in either case, the code will end up in a
// register (which is protected by TMR, in case of TRCH).

#define MAX_STAGES 2 // re-configurable by a macro in Qemu model (requires Qemu rebuild)

// stage -> cmd -> (arm, fire)
static const struct cmd_code stage_cmd_codes[][NUM_SCMDS] = {
    [0] = {
        [SCMD_CAPTURE] = { 0xCD01, 0x01CD },
        [SCMD_LOAD] =    { 0xCD03, 0x03CD },
        [SCMD_CLEAR] =   { 0xCD05, 0x05CD },
    },
    [1] = {
        [SCMD_CAPTURE] = { 0xCD02, 0x02CD },
        [SCMD_LOAD] =    { 0xCD04, 0x04CD },
        [SCMD_CLEAR] =   { 0xCD06, 0x06CD },
    },
};

// cmd -> (arm, fire)
static const struct cmd_code cmd_codes[] = {
    [CMD_DISABLE] = { 0xCD07, 0x07CD },
};

struct hpsc_wdt {
    volatile uint32_t *base; 
    const char *name;
    rtems_vector_number vec;
    hpsc_wdt_cb_t cb;
    void *cb_arg;
    unsigned num_stages;
    bool monitor; // only the monitor can configure the timer
    uint32_t clk_freq_hz;
    unsigned max_div;
    unsigned counter_width;
};

static void exec_cmd(struct hpsc_wdt *wdt, const struct cmd_code *code)
{
    // TODO: Arm-Fire is no protection from a stray jump over here...
    // Are we supposed to separate these with data-driven control flow?
    REGB_WRITE32(wdt->base, REG__CMD_ARM,  code->arm);
    REGB_WRITE32(wdt->base, REG__CMD_FIRE, code->fire);
}

static void exec_global_cmd(struct hpsc_wdt *wdt, enum cmd cmd)
{
    printk("WDT %s: exec cmd: %u\n", wdt->name, cmd);
    assert(cmd < NUM_CMDS);
    exec_cmd(wdt, &cmd_codes[cmd]);
}
static void exec_stage_cmd(struct hpsc_wdt *wdt, enum stage_cmd scmd,
                           unsigned stage)
{
    DPRINTK("WDT %s: stage %u: exec stage cmd: %u\n", wdt->name, stage, scmd);
    assert(stage < MAX_STAGES);
    assert(scmd < NUM_SCMDS);
    exec_cmd(wdt, &stage_cmd_codes[stage][scmd]);
}

static void hpsc_wdt_isr(void *arg)
{
    struct hpsc_wdt *wdt = (struct hpsc_wdt *)arg;
    assert(wdt);
    printk("WDT %s: ISR\n", wdt->name);
    if (wdt->cb)
        wdt->cb(wdt, wdt->cb_arg);
}

static rtems_status_code hpsc_wdt_init(
    struct hpsc_wdt *wdt,
    const char *name,
    volatile uint32_t *base,
    rtems_vector_number intr_vec,
    hpsc_wdt_cb_t cb,
    void *cb_arg,
    bool monitor,
    uint32_t clk_freq_hz,
    unsigned max_div
)
{
    printf("WDT %s: create base %p\n", name, base);
    wdt->base = base;
    wdt->name = name;
    wdt->monitor = false;
    wdt->vec = intr_vec;
    wdt->cb = cb;
    wdt->cb_arg = cb_arg;
    wdt->monitor = monitor;
    if (wdt->monitor) {
        wdt->clk_freq_hz = clk_freq_hz;
        wdt->max_div = max_div;
        wdt->counter_width = (64 - log2_of_pow2(wdt->max_div) - 1);
    }
    return rtems_interrupt_handler_install(wdt->vec, wdt->name, 
                                           RTEMS_INTERRUPT_UNIQUE,
                                           hpsc_wdt_isr, wdt);
}

rtems_status_code hpsc_wdt_probe_monitor(
    struct hpsc_wdt **wdt,
    const char *name,
    volatile uint32_t *base,
    rtems_vector_number intr_vec,
    hpsc_wdt_cb_t cb,
    void *cb_arg,
    uint32_t clk_freq_hz,
    unsigned max_div
)
{
    *wdt = malloc(sizeof(struct hpsc_wdt));
    if (!*wdt) {
        printf("ERROR: WDT: malloc failed\n");
        return RTEMS_NO_MEMORY;
    }
    return hpsc_wdt_init(*wdt, name, base, intr_vec, cb, cb_arg, true,
                         clk_freq_hz, max_div);
}

rtems_status_code hpsc_wdt_probe_target(
    struct hpsc_wdt **wdt,
    const char *name,
    volatile uint32_t *base,
    rtems_vector_number intr_vec,
    hpsc_wdt_cb_t cb,
    void *cb_arg
)
{
    *wdt = malloc(sizeof(struct hpsc_wdt));
    if (!*wdt) {
        printf("ERROR: WDT: malloc failed\n");
        return RTEMS_NO_MEMORY;
    }
    return hpsc_wdt_init(*wdt, name, base, intr_vec, cb, cb_arg, false, 0, 0);
}

int hpsc_wdt_configure(
    struct hpsc_wdt *wdt,
    unsigned freq,
    unsigned num_stages,
    uint64_t *timeouts
)
{
    assert(wdt);
    assert(wdt->monitor);
    assert(!hpsc_wdt_is_enabled(wdt)); // not strict requirement, but for sanity
    if (num_stages > MAX_STAGES) {
        printk("ERROR: WDT: more stages than supported: %u >= %u\n",
               num_stages, MAX_STAGES);
        return 1;
    }
    if (!(freq <= wdt->clk_freq_hz && wdt->clk_freq_hz % freq == 0)) {
        printk("ERROR: WDT: freq is larger than or not a divisor of clk freq: %u > %u\n",
               freq, wdt->clk_freq_hz);
        return 1;
    }
    for (unsigned stage = 0; stage < num_stages; ++stage) {
        if (timeouts[stage] & (~0ULL << wdt->counter_width)) {
                printk("ERROR: WDT: timeout for stage %u exceeds counter width (%u bits): %08x%08x\n",
                       stage, wdt->counter_width,
                      (uint32_t)(timeouts[stage] >> 32), (uint32_t)(timeouts[stage] & 0xffffffff));
                return 2;
        }
    }

    wdt->num_stages = num_stages;

    // Set divider and zero other fields
    assert(wdt->clk_freq_hz % freq == 0);
    unsigned div = wdt->clk_freq_hz / freq;
    if (div > wdt->max_div) {
        printk("ERROR: WDT: divider too large: %u > %u\n",
               div, wdt->max_div);
        return 1;
    }

    printk("WDT %s: set divider to %u\n", wdt->name, div);
    REGB_WRITE32(wdt->base, REG__CONFIG, div << REG__CONFIG__TICKDIV__SHIFT);

    for (unsigned stage = 0; stage < num_stages; ++stage) {
        // Loading alone does not clear the current count (which may have been
        // left over if timer previously enabled and disabled).
        exec_stage_cmd(wdt, SCMD_CLEAR, stage);

        REGB_WRITE64(wdt->base, STAGE_REG(REG__TERMINAL, stage), timeouts[stage]);
        exec_stage_cmd(wdt, SCMD_LOAD, stage);
    }
    return 0;
}

rtems_status_code hpsc_wdt_remove(struct hpsc_wdt *wdt)
{
    rtems_status_code sc;
    assert(wdt);
    printf("WDT %s: destroy\n", wdt->name);
    sc = rtems_interrupt_handler_remove(wdt->vec, hpsc_wdt_isr, wdt);
    if (wdt->monitor)
        assert(!hpsc_wdt_is_enabled(wdt));
    free(wdt);
    return sc;
}

uint64_t hpsc_wdt_count(struct hpsc_wdt *wdt, unsigned stage)
{
    assert(wdt);
    exec_stage_cmd(wdt, SCMD_CAPTURE, stage); 
    uint64_t count = REGB_READ64(wdt->base, STAGE_REG(REG__COUNT, stage));
    printk("WDT %s: count -> 0x%08x%08x\n", wdt->name,
           (uint32_t)(count >> 32), (uint32_t)(count & 0xffffffff));
    return count;
}

uint64_t hpsc_wdt_timeout(struct hpsc_wdt *wdt, unsigned stage)
{
    assert(wdt);
    // NOTE: not going to be the right value if it wasn't not loaded via cmd
    uint64_t terminal = REGB_READ64(wdt->base, STAGE_REG(REG__TERMINAL, stage));
    printk("WDT %s: terminal -> 0x%08x%08x\n", wdt->name,
           (uint32_t)(terminal >> 32), (uint32_t)(terminal & 0xffffffff));
    return terminal;
}

void hpsc_wdt_timeout_clear(struct hpsc_wdt *wdt, unsigned stage)
{
    assert(wdt);
    // TODO: spec unclear: if we are not allowed to clear the int source, then
    // we have to disable the interrupt via the interrupt controller, and
    // re-enable it in hpsc_wdt_enable.
    REGB_CLEAR32(wdt->base, REG__STATUS,
                 1 << ( REG__STATUS__TIMEOUT__SHIFT + stage));
}

bool hpsc_wdt_is_enabled(struct hpsc_wdt *wdt)
{
    bool enabled = REGB_READ32(wdt->base, REG__CONFIG) & REG__CONFIG__EN;
    printk("WDT %s: is enabled -> %u\n", wdt->name, enabled);
    return enabled;
}

void hpsc_wdt_enable(struct hpsc_wdt *wdt)
{
    assert(wdt);
    printk("WDT %s: enable\n", wdt->name);
    REGB_SET32(wdt->base, REG__CONFIG, REG__CONFIG__EN);
}

void hpsc_wdt_disable(struct hpsc_wdt *wdt)
{
    assert(wdt);
    assert(wdt->monitor);
    printk("WDT %s: disable\n", wdt->name);
    exec_global_cmd(wdt, CMD_DISABLE);
}

void hpsc_wdt_kick(struct hpsc_wdt *wdt)
{
    assert(wdt);
    DPRINTK("WDT %s: kick\n", wdt->name);
    // In Concept A variant, there is only a clear for stage 0.  In Concept B
    // variant, there's a clear for each stage, but it is suffient to clear the
    // first stage, because that action has to stop the timers for downstream
    // stages in HW, according to the current interpretation of the HW spec.
    exec_stage_cmd(wdt, SCMD_CLEAR, /* stage */ 0);
}
