/*
Mulitplayer Game Server - Processes Implementation

Author: Tilemachos S. Doganis

See readme.txt for documentation
*/
#include "header.h"

struct items
{
	char name[10];
	int quant;
};

int main(int argc, char *argv[])
{
	/* Variables */
    int sockfd, portno, n, i, j, ready;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char pname[20], cont[40];
	char buffer[256], req[50], chat[256];
	int option;
	struct items player_inv[10];
	int inv_size;
	char inv_file[50];
	char temp[10];
    int read_fdmax;
    fd_set readfd, inp;
	FILE *fp;
	struct timeval tv;

	/* Input Arguments */
	if (argc < 6)
	{
        printf("./player -n <name> -i <inventory> <server_host>\n");
		exit(0);
	}
	else
	{

		while ((option = getopt(argc, argv, "n:i:")) != -1)
		{
			switch (option)
			{
				case 'n': strcpy(pname,optarg);
					break;
				case 'i': strcpy(inv_file,optarg);
					break;
				default: printf("./player –n <name> -i <inventory> <server_host>\n");
					exit(0);
			}		
		}
	}
	
	/* Save Inventory to array player_inv */
	fp = fopen (inv_file, "r");
	i=0;
	if(fp)
	{
		while (fgets(cont,40,fp))
		{
			sscanf(cont,"%s\t%d",player_inv[i].name, &player_inv[i].quant);
			i++;
		}
	}
	fclose(fp);
	portno = 51717;

	/* Create Socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		printf("Could not create socket\n");
		exit(0);
	}
	puts ("Socket Created");

    /* Get server name */
	server = gethostbyname(argv[5]);
	if (server == NULL)
	{
		puts("ERROR, no such host\n");
		exit(0);
	}

	/* Prepare sockaddr_in */
	memset( (char *)&serv_addr, 0, sizeof(serv_addr) );
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr, server->h_addr, server->h_length );
	serv_addr.sin_port = htons(portno);

	/* Connect to remote server */
	if ( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
        perror("Connect Failed\n");
		exit(1);
	}

	puts("Connected\n");

    /* Request Resources from Server*/
	memset(req, '\0', sizeof(req));
	strcpy(req,player_inv[0].name);
	strcat(req,"\t");
	sprintf(temp, "%d", player_inv[0].quant);
	strcat(req,temp);
	j=1;
	while (j<i)
	{
		strcat(req,"\t");
		strcat(req,player_inv[j].name);
		strcat(req,"\t");
		sprintf(temp, "%d", player_inv[j].quant);
		strcat(req,temp);
		j++;
	}
	
	/* Inventory size */
	inv_size = j;
	
	/* Message */
	printf("Requesting: %s\n",req);
	
	/* Send Resource Request */
	n = write(sockfd,req,strlen(req));
	if (n < 0)
 		printf("ERROR writing to socket\n");
	
	/* Request Approval */
    memset(buffer, '\0', 256);
    n = read(sockfd,buffer,256); //'OK' or 'X'
  	if (n < 0)
	    	printf("ERROR reading from socket\n");
	if (strcmp(buffer,"OK") != 0)
		exit (0);
	else
	{
		printf("Inventory Accepted: \n");
		for (j=0; j<inv_size; j++)
			printf("%s %d\n",player_inv[j].name, player_inv[j].quant);
	}
	
	/* Send Player Name */
	n = write(sockfd,pname,strlen(pname));
	if (n < 0)
 		printf("ERROR writing to socket\n");
	
	printf("WAITING FOR GAME TO START\n");

    memset(buffer,'\0',256);
    while (strcmp(buffer,"Game has started\nChat:\n") != 0)
    {
        printf("Waiting for Read\n");
        strcpy(buffer,freadfd(sockfd,0));
        printf("%s",buffer);
        fwritefd(sockfd,"OK",2);
        printf("Client sending OK\n");
    }

	while (1)
	{
        /* Prepare select from stdin (5 sec wait time) */
		FD_ZERO(&inp);
		FD_SET(0, &inp);
        tv.tv_sec = 5;
		tv.tv_usec = 0;

		printf("Please enter message: \n");
		ready = select(1, &inp, NULL, NULL, &tv);
        memset(chat, '\0', 256);
        /* If message is given in 5 seconds, send to server */
		if (ready)
		{
            fflush(stdout);
            fgets(chat,256,stdin);
			/* Send Message */
            fwritefd(sockfd, chat, 0);
		}

        /* Read message from other / same player, if any available within 1 sec */
        memset(chat, '\0', 256);
        FD_ZERO(&readfd);
        FD_SET(sockfd, &readfd);
        read_fdmax = sockfd;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ready = select(read_fdmax+1, &readfd, NULL, NULL, &tv);
        memset(chat, '\0', 256);
        if (ready)
        {
            strcpy(chat,freadfd(sockfd,0));
            printf("%s",chat);
        }

  	}
	return 0;
}
