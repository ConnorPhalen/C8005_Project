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
--
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
void* tProcess(void *arguments);
void readWrite(int src_sock_sock,int dest_sock);
void sigHandler(int sigNum);


struct targs{
	char port1[SMALLBUF];
	char ip[SMALLBUF];
	char port2[SMALLBUF];
};

/*---------------------------------------------------------------------------------------------
--  FUNCTION:           main
--
--  DATE:                   April 6, 2019
--
--  REVISIONS:      (Date and Description)
--  N/A
--
--  DESIGNERS:      Connor Phalen
--
--  PROGRAMMERS:    Connor Phalen
--
--  INTERFACE:      int main (int argc, char **argv)
--  int argc: the number of arguments inputed via command line
--  char ** argv    the arguments inputed via command line
--
--  NOTES:
--  the main use of the main function is to parse the information from the config file
--	and create a thread to run use the parsed information in building sockets
----------------------------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
/* ---- Variable Setup ---- */

	FILE *fr; // For reading the related .conf file
	char rbuf[BUFLEN];
	char *port1,  *token2;

/* ---- Extract .conf File Contents ---- */
	if((fr = fopen(CONF_FILE, "r")) == NULL) // open .conf file
	{
		perror("Failed to Open File");
		exit(1);
	}

	while(fgets(rbuf, sizeof(rbuf), fr)) // read file line-by-line
	{

		pthread_t *newthread = calloc(1, sizeof(pthread_t)); // new thread for each line
		struct targs *args 	 = calloc(1, sizeof(struct targs)); // each thread has a few of arguments

		port1 = strtok(rbuf, "-"); // Port #1
		token2 = strtok(NULL, "-"); // IP-Port Pair #2

		strcpy(args->port1, port1); // get Port 1

		strcpy(args->ip, strtok(token2, ":")); // get IP 2
		strcpy(args->port2, strtok(NULL, ":")); // get Port 2

		if(pthread_create(newthread, NULL, &tProcess, (void *)args) != 0) // make new thread to deal with each pairing
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
	fclose(fr);
	while(1)
	{

	}
	return 0;
}



/*---------------------------------------------------------------------------------------------
--  FUNCTION:           tProcess
--
--  DATE:                   April 6, 2019
--
--  REVISIONS:      (Date and Description)
--  N/A
--
--  DESIGNERS:      Connor Phalen and Greg Little
--
--  PROGRAMMERS:    Connor Phalen and Greg Little
--
--  INTERFACE:      void* tprocess (void* arguments)
--  void* arguments: contains the port and address information
--	arguments->port1, arguments->port2, arguments->ip
--  NOTES:
--  builds a socket for server and sockets for accepting new connections as well as forwarding the data
----------------------------------------------------------------------------------------------*/
void* tProcess(void *arguments)
{
	struct targs *targs = (struct targs*)arguments; // arguments should be equal to what is printed off above, but it is getting weird
	int server_sockd, forwarding_sockd, new_sockd; // Socket variables
	struct sockaddr_in server_info, r_server; // More Socket variables

/* ---- Server Port ---- */
	// Setup each thread to listen to ports. then splice data between each of these sockets, or something like that.
	fprintf(stdout, "Opening server on Port %ld\n", atol(targs->port1));

	// Create socket
	if((server_sockd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		// Error in Socket creation
		perror("server Socket failed to be created");
		exit(1);
	}

	if (setsockopt(server_sockd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
	{
    	perror("setsockopt(SO_REUSEADDR) failed");
	}

	// Zero out and create memory for the server struct
	bzero((char *)&server_info, sizeof(struct sockaddr_in));
	server_info.sin_family = AF_INET;
	server_info.sin_port = htons(atol(targs->port1));  // Flip the bits for the weird intel chip processing
	server_info.sin_addr.s_addr = INADDR_ANY; // Accept addresses from anybody

	// Bind new socket to port
	if(bind(server_sockd, (struct sockaddr *)&server_info, sizeof(server_info)) == -1)
	{
		perror("Cannot bind to socket");
		exit(1);
	}
	while(1){
		listen(server_sockd, LISTEN_QUOTA);// cause we like the NSA over here
/* ---- End Server Port ---- */


		new_sockd = accept(server_sockd,NULL,NULL); // accept the call coming from the server

/* ---- Forwarding Port ---- */
		if((forwarding_sockd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			// Error in Socket creation
			perror("forwarding socket failed to be created");
			exit(1);
		}

		if(setsockopt(forwarding_sockd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
		{
    		perror("setsockopt(SO_REUSEADDR) failed");
		}

		// Zero out and create memory for the server struct
		bzero((char *)&r_server, sizeof(struct sockaddr_in));
		r_server.sin_family = AF_INET;
		r_server.sin_port = htons(atol(targs->port2));  // Flip the bits for the weird intel chip processing
		inet_aton(targs->ip, &r_server.sin_addr);
		// Bind new socket to port
		if(connect(forwarding_sockd, (struct sockaddr *)&r_server, sizeof(r_server)) == -1)
		{
			perror("Cannot bind to socket");
			exit(1);
		}
/* ---- End Forwarding Port ---- */

		if(fork() == 0){//ready to make a baby baby
			if(fork() == 0 ){ // the baby had a baby omg how does this even happen :O
				//younger child do work
				readWrite(new_sockd,forwarding_sockd);//sends the data on one socket to the other
				exit(0);
			}else{
				// older child do work (father?)
				readWrite(forwarding_sockd,new_sockd);//sends the data on other socket to the one
				exit(0);
			}
			exit(0);
		}
	}
	return 0;
}

/*---------------------------------------------------------------------------------------------
--  FUNCTION:           readWrite
--
--  DATE:                   April 6, 2019
--
--  REVISIONS:      (Date and Description)
--  N/A
--
--  DESIGNERS:      Greg Little
--
--  PROGRAMMERS:    Greg Little
--
--  INTERFACE:      void readWrite (int src_sock, int dest_sock)
--  int src_sock: source socket used to get data from in order to send to destination socket
--	int dest_sock:	destintion socket used to send data to destination from source socket
--
--  NOTES:
-- gets data and sends it through sockets
----------------------------------------------------------------------------------------------*/
void readWrite(int src_sock,int dest_sock){
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
}



/*---------------------------------------------------------------------------------------------
--  FUNCTION:           sigHandler
--
--  DATE:                   April 6, 2019
--
--  REVISIONS:      (Date and Description)
--  N/A
--
--  DESIGNERS:      Connor Phalen
--
--  PROGRAMMERS:    Connor Phalen
--
--  INTERFACE:      void sigHandler (int sigNum)
--  int sigNum: used to signify between the different error events
--
--  NOTES:
-- handles signal interupts
----------------------------------------------------------------------------------------------*/
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
