/*
Mulitplayer Game Server - Multithreading Implementation

Author: Tilemachos S. Doganis

See readme.txt for documentation
*/
#include "header.h"

struct items
{
	char name[10];
	int avail;
};

struct items game_inv[10];
int sockfd;
int g_match;
int player_num;
int quota;
int inv_size;
int *g_go = NULL;
pthread_t *mainthreads = NULL;
pthread_mutex_t g_mutex;
pthread_cond_t g_cvar;
pthread_attr_t attr;

/* Struct for passing arguments to client thread */
struct args
{
    int sock;
    int id;
    struct items *inv;
    pthread_mutex_t *s_opmutex;
    pthread_mutex_t *s_ipmutex;
    pthread_mutex_t *s_capmutex;
    pthread_mutex_t *s_rsrmutex;
    pthread_mutex_t *s_phase;
    pthread_cond_t *s_cval;
    int *s_inpipe;
    int *s_outpipe;
    int *s_synch;
    int *s_capacity;
    int *s_fl;
};

void *serve(void *pass)
{
    /* Variables */
    struct args *temp = (struct args *)pass;
    int cli_sock,  n, req_count, k,i ,j;
    int srv_fdmax;
    char buffer[256], pname[20], message[256], chat[256], cont[40];
    char * it;
    struct items rcv_inv[10];
    struct items *resource;
    int to_take[10];
    fd_set srv_read;
    struct timeval tv;
    int ident;
    int qquant;

    /* Receive Arguments from match thread */
    pthread_mutex_t *opmutex = temp->s_opmutex; //Match thread -> Client thread pipe
    pthread_mutex_t *ipmutex = temp->s_ipmutex; //Client thread-> Match thread pipe
    pthread_mutex_t *capmutex = temp->s_capmutex; //Capacity
    pthread_mutex_t *rsrmutex = temp->s_rsrmutex; //Resources
    pthread_mutex_t *phase = temp->s_phase; //Waiting Phase -> Message Phase
    pthread_cond_t *cval = temp->s_cval; //Condition Variables for Phase Change
    int *inpipe = temp->s_inpipe;
    int *outpipe = temp->s_outpipe;
    int *synch = temp->s_synch;
    int *capacity = temp->s_capacity;
    int *fl = temp->s_fl;
    cli_sock = temp->sock;
    ident = temp->id;
    resource = temp->inv;

    /* Receive resource request from client */
    memset(buffer, '\0', 256 );
    n = read(cli_sock, buffer, 256); //req
    if ( n < 0 )
        printf("Error reading from socket\n");

    printf("New Request: %s\n", buffer);

    /* Receive player inventory */
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
    pthread_mutex_lock(capmutex);

    if (check(cli_sock, &capacity,0) == 0)
    {
        /* Exit thread */
        printf("[%d]Clearing Memory and Exiting...",ident);

        printf("[%d] Unlocking CapMutex\n",ident);
        pthread_mutex_unlock(capmutex);
        pthread_exit(NULL);
    }

    pthread_mutex_unlock(capmutex);

   /* Adjust server resources */

    pthread_mutex_lock(rsrmutex);

    qquant = 0;
    for (j=0; j<inv_size; j++)
    {
        qquant = 0;
        to_take[j] = -1;

        /* Check Resource names / Check if quantity < quota */
        for (k=0; k <req_count; k++)
        {
            qquant = qquant + rcv_inv[k].avail;
            if (strcmp(rcv_inv[k].name, resource[j].name) == 0 ) to_take[j] = k;
        }

        /* If problem with resources, cancel connection */
        if (((resource[j].avail - rcv_inv[j].avail) < 0 ) || (to_take[j] == -1) || (qquant > quota))
        {
            printf("[%d]Rejecting Resources\n",ident);
            n = write(cli_sock, "X", 1);
            if ( n < 0 ) printf ("Error writing to socket (Cancel Connection)\n");

            printf("[%d] Unlocking Rsr Mutex\n",ident);
            pthread_mutex_unlock(rsrmutex);
            pthread_exit(NULL);
        }
    }
    /* Update resources */
    for(j=0; j<inv_size; j++)
        resource[j].avail = resource[j].avail - rcv_inv[to_take[j]].avail;

    /* Send Request Approval */
    n = write(cli_sock, "OK", 2);
    if ( n < 0 ) printf ("Error writing to socket\n");

    /* Increase Capacity */
    pthread_mutex_lock(capmutex);
    (*capacity)++;
    pthread_mutex_unlock(capmutex);


    /* Receive Player Name */
    memset(buffer, '\0', 256 );
    n = read(cli_sock, buffer, 256); //pname
    if ( n < 0 ) printf("Error reading from socket\n");
    printf("\nPlayer %s connected to server\n", buffer);
    strcpy(pname,buffer);

    /* Print Current Inventory */
    printf("Current Inventory:\n");
    for (j=0; j<inv_size; j++)
        printf("%s %d\n", resource[j].name, resource[j].avail);
    printf("\n");

    pthread_mutex_unlock(rsrmutex);

    /* Wait for all players to connect */
    do
    {
        printf("[%d]Checking for disconnects\n",ident);
        /* Check for disconnects */

        pthread_mutex_lock(capmutex);

        i = check(cli_sock, &capacity,1);

        pthread_mutex_unlock(capmutex);

       /* If player has disconnected */
       if (i == 0)
       {
           /* Adjust server resources  */
           pthread_mutex_lock(rsrmutex);

            printf("Restoring resources...\n");

            /* Restore resources */
            for(j=0; j<inv_size; j++)
                resource[j].avail = resource[j].avail + rcv_inv[to_take[j]].avail;

            pthread_mutex_unlock(rsrmutex);
            pthread_exit(NULL);
       }

       /* Send wait message to from server parent to client & receive response */
       printf("[%d] Sending Wait Message to Client\n",ident);
       pthread_mutex_lock(opmutex);

       printf("[%d] Trying to read outpipe\n",ident);
       strcpy(cont,freadfd(outpipe[0],0));

       printf("[%d]Reading outpipe: %s \n",ident,cont);
       fwritefd(cli_sock,cont,0);

       strcpy(cont,freadfd(cli_sock,0));
       printf("[%d]Reading from socket: %s\n",ident,cont);

       pthread_mutex_unlock(opmutex);
    }
    while (*capacity < player_num);
    printf("[%d]Thread done wait\n",ident);

    /* Declare Thread end and wait for the rest */
    pthread_mutex_lock(phase);
    (*synch)++;
    if (*synch == player_num) pthread_cond_broadcast(cval);
    else pthread_cond_wait(cval, phase);
    pthread_mutex_unlock(phase);
    printf("[%d]Thread unlocking\n",ident);

    /* Start Game */
    strcpy(buffer,"Game has started\nChat:\n");
    printf("[%d]Sending START\n",ident);
    fwritefd(cli_sock, buffer, 0);
    printf("[%d]START Sent\n",ident);

    /* Clear last OK from each player */
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
        pthread_mutex_lock(capmutex);
        i = check(cli_sock, &capacity,1);
        if (i == 0)
        {
            pthread_mutex_unlock(capmutex);
            pthread_exit(NULL);
            break;
        }
        pthread_mutex_unlock(capmutex);

        /* Prepare select() */
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
           pthread_mutex_lock(ipmutex);
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
           pthread_mutex_unlock(ipmutex);
        }
        /* If parent sends (redirects) message from one player to all */
        else if (FD_ISSET(outpipe[0], &srv_read))
        {
            pthread_mutex_lock(opmutex);
            memset(chat, '\0', 256);

            /* Receive message from parent pipe */
            n = read(outpipe[0],chat,256);
            printf("[%d]Child to Client %s",ident,chat);

            /* Write message to client */
            fwritefd(cli_sock,chat, 0);

            /* Send "Message Received" flag to 0 */
            *fl = 0;
            pthread_mutex_unlock(opmutex);
        }

    } //while(1)

    /* Exit */
    printf("[%d]Exiting...\n",ident);
    pthread_exit(NULL);
}

void *match()
{
    /* Match Variables */
    pthread_mutex_t opmutex;
    pthread_mutex_t ipmutex;
    pthread_mutex_t capmutex;
    pthread_mutex_t rsrmutex;
    pthread_mutex_t phase;
    pthread_cond_t cval;
    int inpipe[2], outpipe[2];
    int synch = 0;
    int *go = &g_go[g_match];
    *go = 0;
    int fl = 0;
    int capacity = 0;
    int soc_fdmax;
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    fd_set soc_read;
    int cli_sock,i, j;
    char  message[256], cont[40];
    struct timeval tv;
    struct items invent[10];
    for (i=0; i<10; i++)
    {
        strcpy(invent[i].name,game_inv[i].name);
        invent[i].avail = game_inv[i].avail;
    }

    /* Thread Preparation */
    pthread_t threads[player_num];
    pthread_mutex_init(&opmutex, NULL);
    pthread_mutex_init(&ipmutex, NULL);
    pthread_mutex_init(&capmutex, NULL);
    pthread_mutex_init(&rsrmutex, NULL);
    pthread_mutex_init(&phase, NULL);
    pthread_cond_init(&cval, NULL);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* Listen */
    listen(sockfd, 5);

    struct args * thr_arg = malloc(sizeof(struct args));

    /* Create server pipes */
    if (pipe(inpipe) != 0) printf("Error creating input pipe");
    if (pipe(outpipe) != 0) printf("Error creating output pipe");

    /* Match loop */
    while ((capacity > 0) || (*go == 0))
    {
        /* Match thread accepts connections and waits for Client thread synchronization */
        while  (synch < player_num)
        {
            printf("Waiting for incoming connections...\n");

            /* Setup Client Select */
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

                /* Prepare Arguments for Thread serving Client */
                thr_arg->sock = cli_sock;
                thr_arg->inv = invent;
                thr_arg->id = capacity;
                thr_arg->s_opmutex = &opmutex;
                thr_arg->s_ipmutex = &ipmutex;
                thr_arg->s_capmutex = &capmutex;
                thr_arg->s_rsrmutex = &rsrmutex;
                thr_arg->s_phase = &phase;
                thr_arg->s_cval = &cval;
                thr_arg->s_inpipe = inpipe;
                thr_arg->s_outpipe = outpipe;
                thr_arg->s_synch = &synch;
                thr_arg->s_capacity = &capacity;
                thr_arg->s_fl = &fl;

                pthread_mutex_lock(&capmutex);
                /* Create New Thread */
                pthread_create(&threads[capacity], &attr, &serve, thr_arg);
                pthread_detach(threads[capacity]);

                pthread_mutex_unlock(&capmutex);
            }
            /* If not all players have connected and synchronized */
            if (synch < player_num)
            {
                /* Send Number of Players Connected to each connected client */
                pthread_mutex_lock(&capmutex);

               sprintf(cont,"%d/%d players connected...\n",capacity,player_num);

               for (i=0; i<capacity; i++)
               {
                   fwritefd(outpipe[1],cont,0);
                   printf("MAIN: Sending players connected\n");
               }
               pthread_mutex_unlock(&capmutex);
             }
            else
            {
                /* Unlock condition variable and start game */
                pthread_mutex_lock(&phase);
                pthread_cond_broadcast(&cval);
                printf("MAIN Unlocking All Threads\n");
                *go = 1;
                pthread_cond_broadcast(&g_cvar);
                pthread_mutex_unlock(&phase);
                printf("Game has started\nChat:\n");
            }
        }


        printf("%d Players Remaining\n",capacity);

        /* Receive messages from child pipe */
        memset(message, '\0', 256);
        strcpy(message,freadfd(inpipe[0],0));
        printf("Parent %s",message);

        /* Write message to child pipes */
        for (i=0; i<capacity; i++) fwritefd(outpipe[1],message, 0);

    } //WHILE(1)
    printf("Exiting...\n");
    close(inpipe[0]);
    close(inpipe[1]);
    close(outpipe[0]);
    close(outpipe[1]);
    close (cli_sock);
    close(sockfd);
    free(thr_arg);
    pthread_mutex_destroy(&opmutex);
    pthread_mutex_destroy(&ipmutex);
    pthread_mutex_destroy(&capmutex);
    pthread_mutex_destroy(&rsrmutex);
    pthread_mutex_destroy(&phase);
    pthread_cond_destroy(&cval);
    return 0;

}

//./gameserver -p <num_of_players> -i <game_inventory> -q <quota_per_player>
int main(int argc, char *argv[])
{
    /* Server Variables for all matches */
    int option;
    FILE *fp;
    char cont[40], inv_file[50];
    int portno,ret,on,i,j;
    struct sockaddr_in serv_addr;

    /* Latest Match Number */
    g_match = 0;

    /* Match threads (dynamic) array */
    mainthreads = malloc(2*sizeof(pthread_t));
    if (mainthreads == NULL) printf("Error in thread malloc\n");

    /* Match start flag (dynamic) array */
    g_go = malloc(2*sizeof(int));
    if (g_go == NULL) printf("Error in vars malloc\n");

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_mutex_init(&g_mutex, NULL);
    pthread_cond_init (&g_cvar, NULL);

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

	/* Create socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) printf("ERROR opening socket\n");

	puts("Socket created\n");

    /* all values in buffer (serv_addr) -> 0 */
	memset( (char*)&serv_addr, 0, sizeof(serv_addr) );
	
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
		printf("Bind Failed\n");
	puts("Bind Done\n");


    while (1)
    {
        pthread_create(&mainthreads[g_match], &attr, &match, NULL);
        pthread_detach(mainthreads[g_match]);

        /* Wait for match to start */
        pthread_mutex_lock(&g_mutex);
        pthread_cond_wait(&g_cvar, &g_mutex);

        /* Create new match */
        g_match++;
        printf("Creating Match %d\n",(g_match+1));

        /* Reallocating dynamic arrays for match threads and match start flags */
        mainthreads = realloc(mainthreads,(2+g_match)*sizeof(pthread_t));
        g_go = realloc(g_go,(2+g_match)*sizeof(int));
        if (mainthreads == NULL) printf("Error Reallocating Threads\n");
        if (g_go == NULL) printf("Error Reallocating vars\n");
        pthread_mutex_unlock(&g_mutex);
    }
    pthread_cond_destroy(&g_cvar);
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&g_mutex);
    close(sockfd);
    free(g_go);
    pthread_join(*mainthreads,NULL);
    free(mainthreads);
    pthread_exit(NULL);

    exit(0);

}
