/*---------------------------------------------------------------------------------------
--	SOURCE FILE:	IPPF.c
--
--	PROGRAM:		IPPF
--
--	FUNCTIONS:		Add Additional Functions Used Here
--
--	DATE:			April 8, 2019
--
--	REVISIONS:		(Date and Description)
--
--				April 1, 2019:
--					- Initial Setup and GitHub Repo
--				April 2, 2019:
--					-
--
--	DESIGNERS:		Connor Phalen and Greg Little
--
--	PROGRAMMERS:	Connor Phalen and Greg Little
--
--	NOTES:
-- - Isaac mentioned a that this can be used for good performance: http://man7.org/linux/man-pages/man2/splice.2.html
-- - Can use tee() to duplicate data out of a socket, so we can essentially port forward from one sock to multiple if we want to
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define CONF_FILE "IPP_Pairs.conf" // Contains IP-Port pairs
#define BUFLEN 1028
#define SMALLBUF 16
#define LISTEN_QUOTA 5

/* ---- Function and struct Setup ---- */
void* tprocess(void *arguments);
void sigHandler(int sigNum);
void readwrite(int src_sock_sock,int dest_sock);
struct targs{
	char port1[SMALLBUF];
	char ip2[SMALLBUF];
	char port2[SMALLBUF];
};

/* Program Start */
int main(int argc, char **argv)
{
/* ---- Variable Setup ---- */
	//int i, maxi, nready;
	//int socket_desc, l_client_len, r_client_len; 	// Socket specific

	//struct sockaddr_in server, client;

	FILE *fr; // For reading the related .conf file
	char rbuf[BUFLEN];
	char *token1,  *token2;

/* ---- Extract .conf File Contents ---- */
	if((fr = fopen(CONF_FILE, "r")) == NULL) // open .conf file
	{
		perror("Failed to Open File");
		exit(1);
	}

	while(fgets(rbuf, sizeof(rbuf), fr)) // read file line-by-line
	{
		printf("Line Characters %s\n", rbuf);

		pthread_t *newthread = calloc(1, sizeof(pthread_t)); // new thread for each line
		struct targs *args 	 = calloc(1, sizeof(struct targs)); // each thread has a few of arguments

		token1 = strtok(rbuf, "-"); // Port #1
		token2 = strtok(NULL, "-"); // IP-Port Pair #2

		strcpy(args->port1, token1); // get Port 1

		strcpy(args->ip2, strtok(token2, ":")); // get IP 2
		strcpy(args->port2, strtok(NULL, ":")); // get Port 2

		printf("%s - %s - %s This Stuff \n", args->port1, args->ip2, args->port2);

		if(pthread_create(newthread, NULL, &tprocess, (void *)args) != 0) // make new thread to deal with each pairing
		{
			if(errno == EAGAIN)
			{
				perror("Error Occured with Thread Creation");
				exit(1);
			}
		}
	}

/* ---- Forwarding Setup ---- */
	signal(SIGINT, sigHandler); // All Signal Interputs get pushed to sigintHandler

	while(1)
	{

	}

	fclose(fr);
	return 0;
}

// tlisten: Thread Process for client connection and data processing
// Input  - String containing the IP:Port pairs to forward with
// Output - N/A
void* tprocess(void *arguments)
{
	struct targs *targs = (struct targs*)arguments; // arguments should be equal to what is printed off above, but it is getting weird
	int l_sockd, r_sockd, new_sockd; // Socket variables
	struct sockaddr_in l_server, r_server; // More Socket variables

	printf("%s - %s - %s ^^^^ This Stuff \n", targs->port1, targs->ip2, targs->port2);

/* ---- Left Port ---- */
	// Setup each thread to listen to ports. then splice data between each of these sockets, or something like that.
	fprintf(stdout, "Opening server on Port %ld\n", atol(targs->port1));

	// Create socket
	if((l_sockd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		// Error in Socket creation
		perror("Socket failed to be created");
		exit(1);
	}

	if (setsockopt(l_sockd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
	{
    	perror("setsockopt(SO_REUSEADDR) failed");
	}

	// Zero out and create memory for the server struct
	bzero((char *)&l_server, sizeof(struct sockaddr_in));
	l_server.sin_family = AF_INET;
	l_server.sin_port = htons(atol(targs->port1));  // Flip the bits for the weird intel chip processing
	l_server.sin_addr.s_addr = INADDR_ANY; // Accept addresses from anybody

	// Bind new socket to port
	if(bind(l_sockd, (struct sockaddr *)&l_server, sizeof(l_server)) == -1)
	{
		perror("Cannot bind to socket");
		exit(1);
	}
	while(1){
		listen(l_sockd, LISTEN_QUOTA);// cause we like the NSA over here

		/*end of left port*/

		new_sockd = accept(l_sockd,NULL,NULL);
		/* ---- Right Port ---- */

		if((r_sockd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			// Error in Socket creation
			perror("RIGHT SOCKET_Socket failed to be created");
			exit(1);
		}

		if(setsockopt(r_sockd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
		{
    		perror("setsockopt(SO_REUSEADDR) failed");
		}

		// Zero out and create memory for the server struct
		bzero((char *)&r_server, sizeof(struct sockaddr_in));
		r_server.sin_family = AF_INET;
		r_server.sin_port = htons(atol(targs->port2));  // Flip the bits for the weird intel chip processing
		inet_aton(targs->ip2, &r_server.sin_addr);
		// Bind new socket to port
		if(connect(r_sockd, (struct sockaddr *)&r_server, sizeof(r_server)) == -1)
		{
			perror("Cannot bind to socket");
			exit(1);
		}
		if(fork() == 0){//ready to make a baby baby
			if(fork() == 0 ){ // the baby had a baby omg how does this even happen :O
				//younger child do work
				readwrite(new_sockd,r_sockd);
				exit(0);
			}else{
				// older child do work?
				readwrite(r_sockd,new_sockd);
				exit(0);
			}
			exit(0);
		}
	}
	return 0;
}

void readwrite(int src_sock,int dest_sock){
	char buf[BUFLEN];
	int a,j,i;

	a = read(src_sock, buf, BUFLEN);

	while (a > 0) {
        i = 0;

        while (i < a) {
            j = write(dest_sock, buf + i, a - i);

            i += j;
        }

        a = read(src_sock, buf, BUFLEN);
	}
	close(src_sock);
	close(dest_sock);
	exit(0);
}



// Handles Signal Interputs
void sigHandler(int sigNum)
{
	switch(sigNum)
	{
		case SIGINT:
			signal(SIGINT, sigHandler); // reset in case later we do more stuff
			printf("\nMain Program Closing...\n");

			exit(0);
			break;
		default:
			break;
	}
}
