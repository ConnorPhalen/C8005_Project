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
#include <arpa/inet.h>
#include <sys/socket.h>

#define CONF_FILE "IPP_Pairs.conf" // Contains IP-Port pairs
#define BUFLEN 512
#define SMALLBUF 16
#define LISTEN_QUOTA 5

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
	ssize_t ret_bytes, n; // returned bytes
	char *bp, t_rbuf[BUFLEN];
	int bytes_to_read;

	int flags; // used for getting/setting flags on sockets
	int l_sockd, r_sockd, new_sockd; // Socket variables
	int l_client_len, r_client_len;
	struct sockaddr_in l_server, l_client, r_server, r_client; // More Socket variables

	printf("%s - %s - %s - %s ^^^^ This Stuff \n", targs->ip1, targs->port1, targs->ip2, targs->port2);

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
	l_server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept addresses from anybody

	// Bind new socket to port
	if(bind(l_sockd, (struct sockaddr *)&l_server, sizeof(l_server)) == -1)
	{
		perror("Cannot bind to socket");
		exit(1);
	}
/* ---- Right Port ---- */
	// Setup each thread to listen to ports. then splice data between each of these sockets, or something like that.
	fprintf(stdout, "Opening server on Port %ld\n", atol(targs->port2));

	// Create socket
	if((r_sockd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		// Error in Socket creation
		perror("Socket failed to be created");
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
	r_server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept addresses from anybody

	// Bind new socket to port
	if(bind(r_sockd, (struct sockaddr *)&r_server, sizeof(r_server)) == -1)
	{
		perror("Cannot bind to socket");
		exit(1);
	}

/* ---- Set Socket Flags ---- */
	if((flags = fcntl(l_sockd, F_GETFL, 0)) < 0) // Get Socket Descriptor flags
	{
		perror("Could not get Sock Desc Flags");
		exit(1);
	}
	if(fcntl(l_sockd, F_SETFL, flags | O_NONBLOCK) < 0) // Set Socket Flag to Non-Block, OR new flag as to not override the other flags
	{
		perror("Could not set Sock Desc Flags");
		exit(1);
	}
	if((flags = fcntl(r_sockd, F_GETFL, 0)) < 0) // Get Socket Descriptor flags
	{
		perror("Could not get Sock Desc Flags");
		exit(1);
	}
	if(fcntl(r_sockd, F_SETFL, flags | O_NONBLOCK) < 0) // Set Socket Flag to Non-Block, OR new flag as to not override the other flags
	{
		perror("Could not set Sock Desc Flags");
		exit(1);
	}

/* ---- Start Listening ---- */

	listen(l_sockd, LISTEN_QUOTA);
	listen(r_sockd, LISTEN_QUOTA);

	while(1)
	{
		l_client_len = sizeof(l_client);
		r_client_len = sizeof(r_client);

		// accept new client 
		if((new_sockd = accept4(l_sockd, (struct sockaddr *) &l_client, &l_client_len, SOCK_NONBLOCK)) != -1) // Port and IP #1
		{
			printf("IP %s connected on port %s\n", inet_ntoa(l_client.sin_addr), targs->port1);

			if(strcmp(inet_ntoa(l_client.sin_addr), targs->ip1) == 0)
			{
				while((ret_bytes = splice(new_sockd, NULL, r_sockd, NULL, BUFLEN, NULL)) != 0) // Will this block?? Also, cause use flag "SPLICE_F_MORE" if we can work it properly
				{
					// Keep splicing socket data until end of input
					n += ret_bytes; // keep track of amount sent, could use later to send bulk amount
				}
				if(ret_bytes == -1)
				{
					perror("Splice Failed"); // if splice doesn't work, sendfile() could be a good alternative
					// Could fail because one file desc here isn't a pipe
					exit(1);
				}
			}
			
		}

		// accept new client 
		if((new_sockd = accept4(r_sockd, (struct sockaddr *) &r_client, &r_client_len, SOCK_NONBLOCK)) != -1) // Port and IP #2
		{
			printf("IP %s connected on port %s\n", inet_ntoa(l_client.sin_addr), targs->port1);

			if(strcmp(inet_ntoa(l_client.sin_addr), targs->ip2) == 0)
			{
				if((ret_bytes = splice(new_sockd, NULL, l_sockd, NULL, BUFLEN, NULL)) == -1) // Will this block?? Also, cause use flag "SPLICE_F_MORE" if we can work it properly
				{
					perror("Splice Failed");
					// Could fail because one file desc here isn't a pipe
					exit(1);
				}
			}

		}
	
/* ---- Test Block ---- *

		if(((new_sockd = accept4(l_sockd, (struct sockaddr *) &l_client, &l_client_len, SOCK_NONBLOCK)) != -1) 
			|| ((new_sockd = accept4(r_sockd, (struct sockaddr *) &r_client, &r_client_len, SOCK_NONBLOCK)) != -1)) // If any connecton between these two
		{
			if((strcmp(inet_ntoa(l_client.sin_addr), targs->ip1) == 0)
				|| (strcmp(inet_ntoa(l_client.sin_addr), targs->ip2) == 0))
			{
				while(1)
				{
					while(splice(new_sockd, NULL, l_sockd, NULL, BUFLEN, NULL) != 0) // if we want non_block -> SPLICE_F_NONBLOCK
					{
						
					}
				}	

			}
		}

*/

/*
		if((new_sockd = accept4(r_sockd, (struct sockaddr *) &r_client, &r_client_len, SOCK_NONBLOCK)) != -1) // Port and IP #2
		{
			printf("IP %s connected on port %s\n", inet_ntoa(l_client.sin_addr), targs->port1);

			if(strcmp(inet_ntoa(l_client.sin_addr), targs->ip2) == 0)
			{
				bp = t_rbuf;
				bytes_to_read = BUFLEN;
				while ((n = read(r_sockd, bp, bytes_to_read)) > 0)
				{
					bp += n;
					bytes_to_read -= n;
				}
				write(l_sockd, t_rbuf, BUFLEN);   // echo to client


			}
		}
*/
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