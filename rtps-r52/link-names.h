#ifndef LINK_NAMES_H
#define LINK_NAMES_H

/*
 * Format is: LINK_NAME__<IFACE>__<REMOTE>_<DIRECTION>
 *
 * For example:
 * LINK_NAME__MBOX__TRCH_SERVER is the local server link for handling mailbox
 * requests from TRCH.
 * LINK_NAME__MBOX__TRCH_CLIENT is the local client link for sending mailbox
 * requests to TRCH.
 */

// Connections to/from TRCH

#define LINK_NAME__MBOX__TRCH_SERVER "LINK_MBOX_TRCH_SERVER"
#define LINK_NAME__MBOX__TRCH_CLIENT "LINK_MBOX_TRCH_CLIENT"

#define LINK_NAME__SHMEM__TRCH_SERVER "LINK_SHMEM_TRCH_SERVER"
#define LINK_NAME__SHMEM__TRCH_CLIENT "LINK_SHMEM_TRCH_CLIENT"

// Connections to/from HPPS

#define LINK_NAME__MBOX__HPPS_SERVER "LINK_MBOX_HPPS_SERVER"
#define LINK_NAME__MBOX__HPPS_CLIENT "LINK_MBOX_HPPS_CLIENT"

#define LINK_NAME__SHMEM__HPPS_SERVER "LINK_SHMEM_HPPS_SERVER"

#endif // LINK_NAMES_H
