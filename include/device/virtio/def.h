#ifndef __KXEMU_DEVICE_VIRTIO_DEF_H__
#define __KXEMU_DEVICE_VIRTIO_DEF_H__

#define VIRTIO_MAGIC   0x74726976
#define VIRTIO_VERSION 0x2
#define VIRTIO_VENDOR  0x554D4551 // "QEMU" in little-endian. Yes, we must use "qemu" to run most of the programs like xv6.

#define VIRTIO_STATUS_ACKNOWLEDGE          1
#define VIRTIO_STATUS_DRIVER               2
#define VIRTIO_STATUS_DRIVER_OK            4
#define VIRTIO_STATUS_FEATURES_OK          8
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET  64
#define VIRTIO_STATUS_FAILED             128

/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT     1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE    2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4

#define VIRTQ_USED_F_NO_NOTIFY 1

#endif
