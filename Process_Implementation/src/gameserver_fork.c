/*
Mulitplayer Game Server - Processes Implementation

Author: Tilemachos S. Doganis

See readme.txt for documentation
*/
#include "header.h"

int mapipe[2];

/* Kill Child Zombie Processes */
void sig_chld(int sig)
{
	pid_t pid;
	int stat;
    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0 )
        printf("Player %d exited with status %d\n",pid, stat);
    signal(SIGCHLD,sig_chld);
}


struct items
{
	char name[10];
	int avail;
};

//./gameserver –p <num_of_players> -i <game_inventory> -q <quota_per_player>
int main(int argc, char *argv[])
{
    /* Server Variables */
    int option, player_num, quota, i, j, inv_size;
    FILE *fp;
    struct items game_inv[10];
    char cont[40], inv_file[50];
    pid_t ffid; //Match Process ID
    struct sockaddr_in serv_addr;
    int portno,ret,on;
    int sockfd;


	/* Input Arguments */
	if (argc < 7)
	{
        printf("./gameserver -p <num_of_players> -i <game_inventory> -q <quota_per_player>\n");
		exit(0);
	}
	else
	{
		while ((option = getopt(argc, argv, "p:i:q:")) != -1)
		{
			switch (option)
			{
				case 'p': player_num = atoi(optarg);
					break;
				case 'i': strcpy(inv_file,optarg);
					break;
				case 'q': quota = atoi(optarg);
					break;
                default: printf("./gameserver -p <num_of_players> -i <game_inventory> -q <quota_per_player>\n");
					exit(0);
			}		
		}
	}
	
	/* Save Inventory to array game_inv */
	fp = fopen (inv_file, "r");
	i = 0;
	if(fp)
	{
		while (fgets(cont,40,fp))
		{
			sscanf(cont,"%s\t%d",game_inv[i].name, &game_inv[i].avail);
			i++;
		}
	}
	fclose(fp);
	
	/* Print Game Specifications */
	printf("Inventory:\n");
	for (j=0; j<i; j++)
		printf("%s %d\n", game_inv[j].name, game_inv[j].avail);
	inv_size=i;
	printf("\nPlayers: %d\n",player_num);
	printf("\nQuota per Player: %d\n",quota);
	
    /* Create socket for bind*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) printf("ERROR opening socket\n");

    puts("Socket created\n");

    /* all values in buffer (serv_addr) -> 0 */
    memset( (char*)&serv_addr, 0, sizeof(serv_addr) );

    /* Define a port number */
    portno = 51717;

    /* Prepare sockaddr_in */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno); //htons: host byte order -> network byte order

    /* Enable address reuse */
    on = 1;
    ret = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );

    /* Bind */
    if ( (ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) ) < 0)
    {
        perror("Bind Failed:");
        exit(0);
    }
    puts("Bind Done\n");
    //signal(SIGINT,sig_int);

    /* Begin match forking loop */
    while(1)
    {
        if (pipe(mapipe) != 0) printf("Error creating match pipe");
        printf("CREATING NEW MATCH\n");
        ffid = fork();

        if (ffid == 0)
        {
            //close(mapipe[0]);

            /* Match Variables */
            int cli_sock,  n, req_count, k;
            int srv_fdmax, soc_fdmax;
            socklen_t clilen;
            pid_t pid;
            char buffer[256], pname[20], message[256], chat[256];
            char * it;
            struct sockaddr_in  cli_addr;
            struct items rcv_inv[10];
            int to_take[10];
            fd_set srv_read, soc_read;
            int inpipe[2], outpipe[2];
            struct timeval tv;
            int ident;
            sem_t my_sem;
            int qquant;

            /* Create Shared Memory for Inventory */
            int shmid_inv = shmget(IPC_PRIVATE,inv_size*sizeof(struct items), 0600);
            struct items *shared_items = (struct items *)shmat(shmid_inv, NULL, 0);
            for (j=0; j<inv_size; j++)
                shared_items[j] = game_inv[j];

            /* Create Shared Memory for Current Capacity */
            int shmid_cap = shmget(IPC_PRIVATE,sizeof(int),0600);
            int *capacity = (int *)shmat(shmid_cap, NULL, 0);
            *capacity = 0;

            /* Create Shared Memory for Game Start Flag */
            int shmid_go = shmget(IPC_PRIVATE,sizeof(int),0600);
            int *go = (int *) shmat(shmid_go, NULL, 0);
            *go = 0;

            /* Create Shared Memory for Message flag received flag*/
            int shmid_fl = shmget(IPC_PRIVATE,sizeof(int),0600);
            int *fl = (int *) shmat(shmid_fl, NULL, 0);
            *fl = 0;

            /* Semaphore Initialization */
            sem_init(&my_sem, 1, 1);

            /* Listen */
            listen(sockfd, 5);

            /* Set default PID to check if child or parent */
            pid = 1;

            /* Create server pipes */
            //Inpipe: Child -> Parent
            //Outpipe: Parent -> Child
            if (pipe(inpipe) != 0) printf("Error creating input pipe");
            if (pipe(outpipe) != 0) printf("Error creating output pipe");

            /* Server Parent loop */
            while ((*capacity > 0) || (*go == 0)) //Go check for initial (accept) loops, capacity check for later (message) loops
            {
                /* Server Parent accepts connections */
                while ((*capacity < player_num) && (pid != 0) && (*go == 0))
                {
                    printf("Waiting for incoming connections...\n");

                    /* Setup Client Select (Wait 5 sec) */
                    FD_ZERO(&soc_read);
                    FD_SET(sockfd, &soc_read);
                    soc_fdmax = sockfd;
                    tv.tv_sec = 5;
                    tv.tv_usec = 0;

                    /* Accept Client */
                    if (select(soc_fdmax+1, &soc_read, NULL, NULL, &tv) > 0)
                    {
                        clilen = sizeof(cli_addr);
                        cli_sock = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                        if (cli_sock < 0)
                        {
                            printf("Accept Failed\n");
                            exit(1);
                        }
                        else puts("Connection Accepted\n");

                        /* Fork */
                        pid = fork();
                        ident = getpid();
                    }

                    /* Send Number of Players Connected to each connected client */
                    if (pid != 0)
                    {
                        sprintf(cont,"%d/%d players connected...\n",*capacity,player_num);
                        for (i=0; i<*capacity; i++) fwritefd(outpipe[1],cont,0);
                    }
                }

                /* When capacity is reached by match process for the first time, begin game */
                if (( *capacity == player_num ) && ( pid != 0 ) && (*go != 1))
                {
                    *go = 1;
                    fwritefd(mapipe[1],"MATCH",5);
                    printf("Game has started\nChat:\n");
                }
                if (pid < 0) printf("ERROR on fork");
                else if (pid == 0)
                {
                    /* Player handling process (Child) */
                    close(sockfd);
                    close(inpipe[0]);
                    close(outpipe[1]);

                    /* Receive resource request from client */
                    memset(buffer, '\0', sizeof(buffer) );
                    n = read(cli_sock, buffer, sizeof(buffer)); //req
                    if ( n < 0 )
                        printf("Error reading from socket\n");
                    printf("New Request: %s\n", buffer);

                    /* Receive player inventory (Decode request string) */
                    j = 0;
                    req_count = 0;
                    it = buffer;
                    int count;
                    while (sscanf(it+j,"%s\t%d\t%n",rcv_inv[req_count].name, &rcv_inv[req_count].avail,&count) > 0)
                    {
                        req_count++;
                        j+=count;
                    }

                    /* Check if potential player exited */
                    if ( check(cli_sock, &capacity,0) == 0)
                    {
                        /* Detach Shared Memory & Exit child */
                        printf("[%d]Clearing Memory and Exiting...",ident);
                        sem_destroy(&my_sem);
                        close(inpipe[1]);
                        close(outpipe[0]);
                        close(cli_sock);
                        shmdt((void *)shared_items);
                        exit(0);
                    }

                   /* CRITICAL SECTION - Adjust server resources */
                    sem_wait(&my_sem);

                    qquant = 0;
                    for (j=0; j<inv_size; j++)
                    {
                        qquant = 0;
                        to_take[j] = -1;

                        /* Check Resource names / Check if quantity < quota */
                        for (k=0; k <req_count; k++)
                        {                            qquant = qquant + rcv_inv[k].avail;
                            if (strcmp(rcv_inv[k].name, shared_items[j].name) == 0 ) to_take[j] = k;
                        }
                        /* If problem with resources, cancel connection */
                        if (((shared_items[j].avail - rcv_inv[j].avail) < 0 ) || (to_take[j] == -1) || (qquant > quota))
                        {
                            printf("[%d]Rejecting Resources: %s\n",ident,rcv_inv[j].name);
                            n = write(cli_sock, "X", 1);
                            if ( n < 0 ) printf ("Error writing to socket (Cancel Connection)\n");
                            shmdt((void *)shared_items);
                            exit(0);
                        }
                    }
                    /* Update resources */
                    for(j=0; j<inv_size; j++)
                        shared_items[j].avail = shared_items[j].avail - rcv_inv[to_take[j]].avail;

                    sem_post(&my_sem);
                    /* END CRITICAL SECTION */

                    /* Send Resource Request Approval to player */
                    n = write(cli_sock, "OK", 2);
                    if ( n < 0 ) printf ("Error writing to socket\n");

                    /* CRITICAL SECTION - Increase capacity */
                    sem_wait(&my_sem);
                    (*capacity)++;
                    sem_post(&my_sem);
                    /* END CRITICAL SECTION

                    /* Receive Player Name */
                    memset(buffer, '\0', sizeof(buffer) );
                    n = read(cli_sock, buffer, sizeof(buffer)); //pname
                    if ( n < 0 ) printf("Error reading from socket\n");
                    printf("\nPlayer %s connected to server\n", buffer);
                    strcpy(pname,buffer);

                    /* Print Current Inventory */
                    printf("Current Inventory:\n");
                    for (j=0; j<inv_size; j++)
                        printf("%s %d\n", shared_items[j].name, shared_items[j].avail);
                    printf("\n");

                    /* Wait for all players to connect */
                    do
                    {
                        printf("[%d]Checking for disconnects\n",ident);
                        /* CRITICAL SECTION - Check for disconnects */
                        sem_wait(&my_sem);
                        i = check(cli_sock, &capacity,1);
                        sem_post (&my_sem);
                        /* END CRITICAL SECTION */

                       /* If player has disconnected */
                       if (i == 0)
                       {
                           /* CRITICAL SECTION - Adjust server resources  */
                            sem_wait(&my_sem);

                            printf("Restoring resources...\n");

                            /* Restore resources */
                            for(j=0; j<inv_size; j++)
                                shared_items[j].avail = shared_items[j].avail + rcv_inv[to_take[j]].avail;

                            sem_post(&my_sem);
                            /* END CRITICAL SECTION */

                           /* Detach shared resources & Exit */
                           close(inpipe[1]);
                           close(outpipe[0]);
                           close(cli_sock);
                           shmdt((void *)shared_items);
                           exit(0);
                       }

                       /* Send wait message to from server parent to client & receive response */
                       strcpy(cont,freadfd(outpipe[0],0));
                       fwritefd(cli_sock,cont,0);
                       strcpy(cont,freadfd(cli_sock,0));
                    }
                    while (*go == 0);

                    /* Start Game */
                    strcpy(buffer,"Game has started\nChat:\n");
                    fwritefd(cli_sock, buffer, 0);
                    printf("[%d]START Sent\n",ident);

                    /* Read & Clear last OK from each player */
                    FD_ZERO(&srv_read);
                    FD_SET(cli_sock, &srv_read);
                    srv_fdmax = cli_sock;
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    if (select(srv_fdmax+1, &srv_read, NULL, NULL, &tv) > 0)
                        n = read(cli_sock,cont,256);

                    /* Waiting for messages */
                    while  (1)
                    {
                        /* Check for disconnects */
                        sem_wait(&my_sem);
                        i = check(cli_sock, &capacity,1);
                        sem_post(&my_sem);
                        if (i == 0) break;
                        /* END CRITICAL SECTION

                        /* Prepare select() for read from parent pipe / player pipe */
                        FD_ZERO(&srv_read);
                        FD_SET(cli_sock, &srv_read);
                        FD_SET(outpipe[0], &srv_read);
                        if (cli_sock > outpipe[0]) srv_fdmax = cli_sock;
                        else srv_fdmax = outpipe[0];
                        memset(buffer, '\0', 256);
                        memset(message, '\0', 256);

                        printf("[%d]Checking for incoming messages..\n",ident);
                        n = select(srv_fdmax+1, &srv_read, NULL, NULL, NULL);

                        /* If no messages received yet & player sends one */
                        if  ( (*fl == 0) && FD_ISSET(cli_sock, &srv_read) )
                        {
                           /* Read Message from player */
                           n = read(cli_sock, buffer, 256);
                           if  (n < 0) perror("Error reading from socket\n");

                           /* Clear empty message when player disconnects */
                           if (strlen(buffer) == 0) continue;

                           /* Set "message received" flag to 1 */
                           *fl = 1;

                           printf("[%d]Client to Child %s",ident,buffer);

                           /* Place name on message */
                           memset(message,'\0',256);
                           strcat(message,pname);
                           strcat(message,": ");
                           strcat(message,buffer);

                           /* Send message to parent pipe */
                           fwritefd(inpipe[1],message, 0);
                        }
                        /* If parent sends (redirects) message from one player to all */
                        else if (FD_ISSET(outpipe[0], &srv_read))
                        {
                            memset(chat, '\0', 256);

                            /* Receive message from parent pipe */
                            n = read(outpipe[0],chat,256);
                            printf("[%d]Child to Client %s",ident,chat);

                            /* Write message to client */
                            fwritefd(cli_sock,chat, 0);

                            /* Send "Message Received" flag to 0 */
                            *fl = 0;
                        }
                    } //while(1)

                    /* Detach Shared Resources & Exit child */
                    printf("[%d]Detaching Resources and Exiting...\n",ident);
                    sem_destroy(&my_sem);
                    close(inpipe[1]);
                    close(outpipe[0]);
                    close(cli_sock);
                    shmdt((void *)shared_items);
                    exit(0);

                } //IF child

                /* Parent (Match process) */
                    close (cli_sock);

                printf("%d Players Remaining\n",*capacity);

                close(inpipe[1]);
                close(outpipe[0]);

                /* Receive messages from child pipe */
                memset(message, '\0', 256);
                strcpy(message,freadfd(inpipe[0],0));
                printf("Parent %s",message);

                /* Write message to child pipes */
                for (i=0; i<*capacity; i++) fwritefd(outpipe[1],message, 0);

                /* Apply Handler to Kill Exited "Zombie" Child Processes */
                signal(SIGCHLD,sig_chld);
            } //WHILE(1)

            printf("Exiting...\n");
            close(inpipe[0]);
            close(outpipe[1]);
            close (cli_sock);
            shmdt((void *)shared_items);
            shmdt((void *)fl);
            shmdt((void *)capacity);
        } //MATCH SERVER
        else
            close(mapipe[1]);
            printf("BEGIN WAIT FOR MATCH\n");
            strcpy(cont,freadfd(mapipe[0],0));
            while(strcmp(cont,"MATCH") != 0)
                strcpy(cont,freadfd(mapipe[0],0));

            printf("WAIT ENDED: %s\n",cont);
    }
    close(mapipe[0]);
	return 0;
}
