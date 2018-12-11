/*
Mulitplayer Game Server - Processes Implementation

Author: Tilemachos S. Doganis

See readme.txt for documentation
*/
#ifndef _freadfd_c_
#define _freadfd_c_
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

const char * freadfd(int fd, int timeout)
{
	fd_set read_set;
	int fdmax, n;
	char * message = NULL;
	message = malloc(sizeof(char)*256);
	struct timeval tv;
	
    memset(message,'\0',256);
	FD_ZERO(&read_set);
	FD_SET(fd, &read_set);
	fdmax = fd;	
		
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	
	if (timeout > 0) n = select(fdmax+1, &read_set, NULL, NULL, &tv);
	else n = select(fdmax+1, &read_set, NULL, NULL, NULL);
    if (n < 0) printf("(freadfd) Read Select Error\n");
	if ( FD_ISSET(fd, &read_set) )
	{	
            n = read(fd,message,256);
            if (n < 0) printf("Error Reading\n");

	}
	return message;
}

#endif
