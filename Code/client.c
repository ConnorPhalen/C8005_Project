/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	client.c
--
--	PROGRAM:		clnt
--
--	FUNCTIONS:		Add Additional Functions Used Here
--
--	DATE:			February 11, 2019
--
--	REVISIONS:		(Date and Description)
--
--				February 11, 2019:
--					- Initial Setup and Push to GitHub Repo
--
--	DESIGNERS:		Connor Phalen and Greg Little
--
--	PROGRAMMERS:	Connor Phalen and Greg Little
--
--	NOTES:
--	Compile using this -> gcc -Wall -o client client.c -lpthread
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

// #define SERVER_LISTEN_PORT 8080
#define PERFFILE "client_perf.csv"
#define BUFLEN 1024
#define PROCMOD 4 // Adjusts spacing between sends, higher numbers is higher delay

struct targs{
    char* host;
    char* work;
};

int connectionWorker (char* host, char* work);
void closeProc(int sigNum);

int send_amount = PROCMOD; // The higher the number, the more overlap between sleep sections, can be local if we delete thread function
pthread_mutex_t *filelock; 

// Program Start
int main(int argc, char **argv)
{
    int number_client = 1;
    char  *host;
    char  work[BUFLEN];

    int SERVER_LISTEN_PORT = 8080;

    //worker
    int socket_desc;
    struct hostent	*hp;
    struct sockaddr_in server;
    int bytes_to_read, n;
    char *bp;
    
	switch(argc)
	{
        case 3:
            host = argv[1];
            SERVER_LISTEN_PORT = strtol(argv[2],NULL,10);
            break;
        case 4:
            host = argv[1];
            SERVER_LISTEN_PORT = strtol(argv[2],NULL,10);
            send_amount = strtol(argv[3],NULL,10);
            break;
		default:
			fprintf(stderr, "Usage: %s [host ip] [host port] (Optional Flags: [times to send])\n", argv[0]);
            exit(1);
	}

    char send_buf[BUFLEN], recieve_buf[BUFLEN];

    printf("num: %d\n",number_client);
	// Create new socket;
    //sending socket_desc to thread
    //if(strcmp(work,"p")==0){
    printf("enter text to send: ");
    fgets(work,BUFLEN,stdin);
    work[strlen(work)-1] = '\0';
    //}

    filelock = calloc(1, sizeof(pthread_mutex_t));

    if(pthread_mutex_init(*(&filelock), NULL) != 0)
    {
        perror("Error initializing thread mutex");
        exit(1);
    }

    signal(SIGINT, closeProc); // Just to guarantee the forked proc is closed with Ctrl+C

    pid_t forker;

    for (int i=1; i< number_client; i++){
        if((forker = fork()) == 0){
          break;
        }else if(forker<0){
            i--;
        }
    }

    //connectionWorker
    if((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Cannot create socket");
        exit(1);
    }
    // Zero out and create memory
    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_LISTEN_PORT); // Flip those connection bits

/* Non-Blocking Test */
    int flags;
    if((flags = fcntl(socket_desc, F_GETFL, 0)) < 0) // Get Socket Descriptor flags
    {
        perror("Could not get Sock Desc Flags");
        exit(1);
    }
    if(fcntl(socket_desc, F_SETFL, flags | O_NONBLOCK) < 0) // Set Socket Flag to Non-Block, OR new flag as to not override the other flags
    {
        perror("Could not set Sock Desc Flags");
        exit(1);
    }

    // Get name of host
    if((hp = gethostbyname(host)) ==  NULL)
    {
        fprintf(stderr, "Unknown server address\n");
        exit(1);
    }
    bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

    if(connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        fprintf(stderr, "Failed to connect to server\n");
        perror("connect");
        exit(1);
    }

    while(1)
    {
        strcpy(send_buf,work);

        send(socket_desc, send_buf, BUFLEN, 0);

        bp = recieve_buf;
        bytes_to_read = BUFLEN;

        bzero(bp, BUFLEN);
        n=0;
        // Keep receiving until no more data on socket
        while((n = recv(socket_desc, bp, bytes_to_read, 0)) < bytes_to_read)//change bytes_to_read
        {
          bp += n;
          bytes_to_read -= n;
        }

        printf("Message Recieved From Server - R:%s\n", recieve_buf);
        sleep(1); // Temporary testing call
    }
/*
    for(int i=0; i<send_amount; i++){
        strcpy(send_buf,work);

        send(socket_desc, send_buf, BUFLEN, 0);

        bp = recieve_buf;
        bytes_to_read = BUFLEN;

        bzero(bp, BUFLEN);
            n=0;
        // Keep receiving until no more data on socket
        while((n = recv(socket_desc, bp, bytes_to_read, 0)) < bytes_to_read)//change bytes_to_read
        {
          bp += n;
          bytes_to_read -= n;
        }

        printf("Message Recieved From Server - R:%s\n", recieve_buf);
    }
*/

    // Clear stdout
    fflush(stdout);
    //close socket
    close(socket_desc);

    int status = 0;
    if(forker==0){
        //printf("hello");
        exit(0);
        while((forker=wait(&status))>0); // will make only main process wait for X time, could make signal based instead
    }
    printf("its over\n");
    free(filelock);
	return(0);
}

void closeProc(int sigNum)
{
    printf("Closing Process...\n");
    exit(0);
}