/*
 * message_slot.h
 *
 *  Created on: 18 Dec 2018
 *      Author: mayam
 */

#ifndef MESSAGE_SLOT_H_
#define MESSAGE_SLOT_H_

#include <linux/ioctl.h>

#define MAJOR_NUM 244
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_RANGE_NAME "message_slot"
#define SUCCESS 0
#define BUFFER_LENGTH 128

#endif /* MESSAGE_SLOT_H_ */
