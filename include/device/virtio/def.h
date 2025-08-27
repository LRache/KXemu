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

// Virtio BLOCK
#define VIRTIO_BLK_DEVICE_ID 2

#define VIRTIO_BLK_F_SIZE_MAX      1
#define VIRTIO_BLK_F_SEG_MAX       2
#define VIRTIO_BLK_F_GEOMETRY      4
#define VIRTIO_BLK_F_RO            5
#define VIRTIO_BLK_F_BLK_SIZE      6
#define VIRTIO_BLK_F_FLUSH         9
#define VIRTIO_BLK_F_TOPOLOGY     10
#define VIRTIO_BLK_F_CONFIG_WCE   11
#define VIRTIO_BLK_F_DISCARD      13
#define VIRTIO_BLK_F_WRITE_ZEROES 14

#define VIRTIO_BLK_T_IN            0
#define VIRTIO_BLK_T_OUT           1
#define VIRTIO_BLK_T_FLUSH         4
#define VIRTIO_BLK_T_GET_ID        8
#define VIRTIO_BLK_T_GET_LIFETIME 10
#define VIRTIO_BLK_T_DISCARD      11
#define VIRTIO_BLK_T_WRITE_ZEROES 13
#define VIRTIO_BLK_T_SECURE_ERASE 14


#endif
