/*
 * message_reader.c
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

static char receive[BUFFER_LENGTH];

int main(int argc, char **argv ){
	int ret, fd;

	receive[BUFFER_LENGTH-1]='\0';

	/*
	 * argv[1] = message slot file path
	 * argv[2] = target message channel id
	 */
	// open message slto device file
	fd = open(argv[1], O_RDONLY);
	if (fd < 0){
		// perror("Failed to open the device...");
		return -1;
	}

	// Set channel id
	ret = ioctl(fd,MSG_SLOT_CHANNEL, atoi(argv[2]));
	if (ret < 0){
		return -1;
	}

	// read a message from the device to a buffer
	ret = read(fd, receive, BUFFER_LENGTH);
	if(ret<0){
		return -1;
	}

	// close the device
	ret = close(fd);
	if (ret < 0){
		// perror("Failed to close the device.");
		return -1;
	}
	printf("%s\n",receive);

	// print status message
	printf("message_reader: reading completed successfully\n");

	// 0 for success. otherwise- nonzero
	return 0;
}
