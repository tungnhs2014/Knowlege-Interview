/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _ABIDEMO_UAPI_H
#define _ABIDEMO_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define ABIDEMO_IOC_MAGIC	'a'

struct abidemo_config {
	__u32 version;
	__u32 mode;
	__u32 flags;
	__u32 reserved[5];
};

#define ABIDEMO_IOC_GET_CONFIG	_IOR(ABIDEMO_IOC_MAGIC, 0x01, struct abidemo_config)
#define ABIDEMO_IOC_SET_CONFIG	_IOW(ABIDEMO_IOC_MAGIC, 0x02, struct abidemo_config)
#define ABIDEMO_IOC_CLEAR_EVENT	_IO(ABIDEMO_IOC_MAGIC, 0x03)

#endif /* _ABIDEMO_UAPI_H */
