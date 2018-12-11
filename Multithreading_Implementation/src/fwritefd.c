/*
Mulitplayer Game Server - Multithreading Implementation

Author: Tilemachos S. Doganis

See readme.txt for documentation
*/
#ifndef _fwritefd_c_
#define _fwritefd_c_
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

void fwritefd(int fd, const char *message, int timeout)
{
	fd_set write_set;
	int fdmax, n;
	struct timeval tv;
    size_t size = strlen(message);

	FD_ZERO(&write_set);
	FD_SET(fd, &write_set);
	fdmax = fd;		
	
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	
	if (timeout > 0) n = select(fdmax+1, NULL, &write_set, NULL, &tv);
	else n = select(fdmax+1, NULL, &write_set, NULL, NULL);
    if (n < 0) printf("(fwritefd)Write Select Error\n");
	if ( FD_ISSET(fd, &write_set) )
	{	
        n = write(fd,message,256);
		if (n < 0) printf("Error Writing");

	}
}

#endif
