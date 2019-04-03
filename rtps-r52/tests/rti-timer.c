#include <rtems.h>

// bist
#include <hpsc-rti-timer-test.h>

// plat
#include <hwinfo.h>

#include "gic.h"
#include "test.h"

int test_core_rti_timer()
{
    rtems_vector_number vec = gic_irq_to_rvn(PPI_IRQ__RTI_TIMER,
                                             GIC_IRQ_TYPE_PPI);

    // Test only one timer, because each timer can be tested only from
    // its associated core, and this BM code is not SMP.
    return hpsc_rti_timer_test(RTI_TIMER_RTPS_R52_0__RTPS_BASE, vec,
                               RTI_MAX_COUNT);
}
