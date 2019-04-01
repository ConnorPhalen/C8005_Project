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
#include <sys/socket.h>

#define CONF_FILE "IPPF.conf" // Contains IP-Port pairs


/* Program Start */
int main(int argc, char **argv)
{
/* ---- Variable Setup ---- */
	int i, maxi, nready;
	int socket_desc, client_len; 	// Socket specific
	int maxfd;			// Select specific
	int clientfd[FD_SETSIZE2];

	struct sockaddr_in server, client;

	FILE *fr; // For reading the related .conf file

/* ---- Forwarding Setup ---- */

}