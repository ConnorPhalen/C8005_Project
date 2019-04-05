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
-- - 
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>

#define CONF_FILE "IPP_Pairs.conf" // Contains IP-Port pairs
#define BUFLEN 512
#define SMALLBUF 16

/* ---- Function and struct Setup ---- */
void* tprocess(void *arguments);
void sigHandler(int sigNum);

struct targs{
	char ip1[SMALLBUF];
	char port1[SMALLBUF];
	char ip2[SMALLBUF];
	char port2[SMALLBUF];
};

/* Program Start */
int main(int argc, char **argv)
{
/* ---- Variable Setup ---- */
	int i, maxi, nready;
	int socket_desc, client_len; 	// Socket specific

	struct sockaddr_in server, client;

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

		token1 = strtok(rbuf, "-"); // IP-Port Pair #1
		token2 = strtok(NULL, "-"); // IP-Port Pair #2

		strcpy(args->ip1, strtok(token1, ":")); // get IP 1
		strcpy(args->port1, strtok(NULL, ":")); // get Port 1

		strcpy(args->ip2, strtok(token2, ":")); // get IP 2
		strcpy(args->port2, strtok(NULL, ":")); // get Port 2

		printf("%s - %s - %s - %s This Stuff \n", args->ip1, args->port1, args->ip2, args->port2);

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

	printf("%s - %s - %s - %s ^^^^ This Stuff \n", targs->ip1, targs->port1, targs->ip2, targs->port2);



	// Setup each thread to listen to ports. then splice data between each of these sockets, or something like that.



	while(1)
	{

	}

	return 0;
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