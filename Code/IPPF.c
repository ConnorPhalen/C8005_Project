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

	while(fgets(rbuf, sizeof(rbuf), fr)) // Read .conf file line-by-line
	{

		pthread_t *newthread = calloc(1, sizeof(pthread_t)); // New thread for each forwarding 
		struct targs *args 	 = calloc(1, sizeof(struct targs)); // Each thread needs to have a few variables passed over

		token1 = strtok(rbuf, "-"); // Port #1
		token2 = strtok(NULL, "-"); // IP-Port Pair to forwaard to

		strcpy(args->port1, token1); // get Port 1

		strcpy(args->ip2, strtok(token2, ":")); // get IP 2
		strcpy(args->port2, strtok(NULL, ":")); // get Port 2

		if(pthread_create(newthread, NULL, &tprocess, (void *)args) != 0) // make new thread to deal with each pairing
		{
			if(errno == EAGAIN)
			{
				perror("Error Occured with Thread Creation");
				exit(1);
			}
		}
	}

	signal(SIGINT, sigHandler); // All Signal Interputs get pushed to sigintHandler
	fclose(fr); // Close thise, as we do not need to read the .conf file anymore

	while(1)
	{
		// loop here to prevent thread closure due to parent's death
	}
	return 0;
}

// tlisten: Thread Process for client connection and data processing
// Input  - String containing the IP:Port pairs to forward with
// Output - N/A
void* tprocess(void *arguments)
{
	struct targs *targs = (struct targs*)arguments; // store submitted args
	int l_sockd, r_sockd, new_sockd; // Socket variables
	struct sockaddr_in l_server, r_server; // More Socket variables

	fprintf(stdout, "Listening on Port %ld\n", atol(targs->port1));

	// Create listening socket
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
		listen(l_sockd, LISTEN_QUOTA); // Setup main port listening socket

		new_sockd = accept(l_sockd,NULL,NULL);
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
		inet_aton(targs->ip2, &r_server.sin_addr); // Specify only the IP to forward to

		// Connect 
		if(connect(r_sockd, (struct sockaddr *)&r_server, sizeof(r_server)) == -1)
		{
			perror("Cannot bind to socket");
			exit(1);
		}
		if(fork() == 0){ // Forked Process will deal with forwarding data
			if(fork() == 0 ){  // Two new processes, each will deal with one flow/direction
				readwrite(new_sockd,r_sockd); // Forward from connecting machine to destination
				exit(0);
			}else{
				readwrite(r_sockd,new_sockd);// Forward from destination to connecting machine
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

	a = read(src_sock, buf, BUFLEN); // read in data from source socket

	while (a > 0) { // a == 0 when at end of input
        i = 0;

        while (i < a) {
            j = write(dest_sock, buf + i, a - i); // write data to forward dest socket

            i += j;
        }
        a = read(src_sock, buf, BUFLEN); // keep reading data,a s there may be more data
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
			// Need to get it so we can close the calloc'd threads on CTRL+C shutdown

			exit(0);
			break;
		default:
			break;
	}
}
