/*
Mulitplayer Game Server - Multithreading Implementation

Author: Tilemachos S. Doganis

See readme.txt for documentation
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/sem.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include "check.c"
#include "freadfd.c"
#include "fwritefd.c"
