/*
Mulitplayer Game Server - Processes Implementation

Author: Tilemachos S. Doganis

See readme.txt for documentation
*/
#ifndef _check_c_
#define _check_c_
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>

int check(int fd, int ** var,int change)
{
	// use the poll system call to be notified about socket status changes
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN | POLLHUP | POLLRDNORM;
	pfd.revents = 0;
	// call poll with a timeout of 100 ms
	if (poll(&pfd, 1, 100) > 0)
	{
		// if result > 0, this means that there is either data available on the
		// socket, or the socket has been closed
		char buffer[32];
		if (recv(fd, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT) == 0)
		{
			// if recv returns zero, that means the connection has been closed:
            printf("Connection Terminated\n");
            if (change == 1) (**var)--;
            close(fd);
            return 0;
		}
	}
    return 1;
}

#endif
