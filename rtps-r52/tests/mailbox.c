#include <assert.h>
#include <string.h>

// plat
#include <mailbox-map.h>

// drivers
#include <hpsc-mbox.h>

// drivers-selftest
#include <hpsc-mbox-test.h>

// libhpsc
#include <devices.h>
#include <link-mbox.h>
#include <link.h>
#include <hpsc-msg.h>

#include "test.h"

#define SEND_RCV_TIME_OUT 3567587328

int test_mbox_lsio_loopback(void)
{
    enum hpsc_mbox_test_rc rc;
    struct hpsc_mbox *mbox_lsio = dev_get_mbox(DEV_ID_MBOX_LSIO);
    assert(mbox_lsio);
    test_begin("test_mbox_lsio_loopback");
    rc = hpsc_mbox_chan_test(mbox_lsio,
                             LSIO_MBOX0_CHAN__RTPS_R52_LOCKSTEP_SSW__LOOPBACK,
                             /* owner */ 0, /* src */ 0, /* dest */ 0);
    test_end("test_mbox_lsio_loopback", rc);
    return rc;
}

int test_mbox_rtps_hpps(unsigned mbox_in, unsigned mbox_out){

    enum hpsc_mbox_test_rc rc = HPSC_MBOX_TEST_SUCCESS;

    test_begin("test_mbox_rtps_hpps");

    if (mbox_in >= HPSC_MBOX_CHANNELS || mbox_out >= HPSC_MBOX_CHANNELS) {
        rc = HPSC_MBOX_TEST_CHAN_CLAIM;
        test_end("test_mbox_rtps_hpps",rc);
        return rc;
    }

    struct hpsc_mbox *mbox_hpps = dev_get_mbox(DEV_ID_MBOX_HPPS_RTPS); 

    assert(mbox_hpps);

    const char* name = "TEST_LINK";
    struct link *test_link = link_mbox_connect(name, mbox_hpps,mbox_in ,mbox_out , 0, 1); 
    assert(link);

    int bytes; //bytes read 

    char write_buf [HPSC_MSG_SIZE];
    char read_buf [HPSC_MSG_SIZE];
    char payload[HPSC_MSG_PAYLOAD_SIZE] = { 0 };

    hpsc_msg_ping(write_buf, HPSC_MSG_SIZE, payload, sizeof(payload));


    bytes = link_request(test_link, 
                     SEND_RCV_TIME_OUT, write_buf, sizeof(write_buf),
                     SEND_RCV_TIME_OUT, read_buf, sizeof(read_buf),
                     RTEMS_EVENT_2);

    //send failure/timeout
    if (bytes == -1) {
        rc = HPSC_MBOX_TEST_CHAN_WRITE;
    }
    //receive timeout
    else if (bytes == -2) {
        rc = HPSC_MBOX_TEST_CHAN_NO_RECV;
    }

    link_disconnect(test_link);
    test_end("test_mbox_rtps_hpps",rc);
    return rc;
}
