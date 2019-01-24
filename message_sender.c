/*
 * message_sender.c
 *
 *  Created on: 18 Dec 2018
 *      Author: mayam
 */

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include <sys/ioctl.h>
#include "message_slot.h"

int main(int argc, char **argv ){
/*
 * argv[1] = message slot file path
 * argv[2] = target message channel id
 * argv[3] = the message
 */
	int ret, fd;
	// open message slto device file
	fd = open(argv[1], O_WRONLY);
	if (fd < 0){
		// perror("Failed to open the device...");
		return -1;
	}

	// Set channel id
	ret = ioctl(fd,MSG_SLOT_CHANNEL, atoi(argv[2]));
	if (ret < 0){
		return -1;
	}

	// Write specified message to the file
	ret = write(fd, argv[3], strlen(argv[3]));
	if (ret < 0){
	      // perror("Failed to write the message to the device.");
	      return -1;
	   }

	// close the device
	ret = close(fd);
	if (ret < 0){
	      // perror("Failed to close the device.");
	      return -1;
	   }

	// print status message
	printf("message_sender: writing completed successfully\n");

	// 0 for success. otherwise- nonzero

	return 0;
}
