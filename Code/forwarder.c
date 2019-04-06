/*---------------------------------------------------------------------------------------
--	SOURCE FILE:		forwarder.c -   A simple port forwarder using TCP
--
--	PROGRAM:			forward.exe
--
--	FUNCTIONS:
--
--	DATE:				April 5, 2019
--
--	REVISIONS:			(Date and Description)
--
--	DESIGNERS:			Greg Little and Connor Phalen
--
--	PROGRAMMER:		Greg Little And Connor Phalen
--
--	NOTES:
--	to be written later ********
---------------------------------------------------------------------------------------*/

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define EPOLL_QUEUE_LEN	256
#define BUFLEN		80
#define FILEPATH  "config.txt"

//global
int fd_server;
char address_port[10][15] = {'\0'};

//functions
static void SystemFatal(const char* message);
static int ClearSocket (int fd);
void close_fd(int);
void getForwardingInfo();

//main calls functions
int main (int argc, char* argv[]){
  int src_port, dest_port;
  char src_address, dest_address;
  char address[10][15] = {'\0'},ports[10][6]={'\0'};

  getForwardingInfo();
  printf("inital\n");
//  printf("a:%s\n",address_port);
  for(int i=0,k=0,h=0; i< 10; i++){
    for(int j=0; j < 15; j++){
      if(i % 2 == 0){
        address[k][j] = address_port[i][j];
        //printf("%c",address[k][j]);
      }else if(i % 2 != 0 && j<6){
        ports[h][j] = address_port[i][j];
        //printf("%c",ports[h][j]);
      }
      if(address_port[i][j] == '\0'){
      //  printf("break\n");
      //  break;
      }
    }
    if(i % 2 == 0){
      printf("a: %s\n",address[k]);
      k++;
    }else{
      printf("b: %s\n",ports[h]);
      h++;
    }
  }
  //systemSetup()
}

int systemSetup (int port1, int port2, int address1, int address2){
  //will be where all the code to start and setup server info will go
  int arg;
  int num_fds, fd_new, epoll_fd;
  static struct epoll_event events[EPOLL_QUEUE_LEN], event;
  struct sockaddr_in addr, remote_addr;
  struct sigaction act;
  socklen_t addr_size = sizeof(struct sockaddr_in);




  //set up signal handler to close on CTRL-C
  act.sa_handler = close_fd;
  act.sa_flags = 0;
  if(( sigemptyset (&act.sa_mask) == -1 || sigaction (SIGINT, &act, NULL) == -1))
  {
      perror ("failed to set SIGINT handler");
      exit(EXIT_FAILURE);
  }

  fd_server = socket (AF_INET, SOCK_STREAM, 0);
  if (fd_server == -1)
      SystemFatal("socket");

  //create listening socklen_t
  fd_server = socket (AF_INET, SOCK_STREAM,0);
  if (fd_server == -1)
      SystemFatal("socket");

  // set SO REUSEADDR so port can be reused when program closed
  arg = 1;
  if (setsockopt (fd_server, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
      SystemFatal ("setsockopt");

  //server listening socket non blocking
  if (fcntl (fd_server, F_SETFL, O_NONBLOCK | fcntl (fd_server, F_GETFL, 0)) == -1 )
      SystemFatal ("fcntl");

  //bind to specific src_port
  memset (&addr, 0, sizeof( struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port1);
  if (bind (fd_server, (struct sockaddr*) &addr, sizeof(addr)) == -1)
      SystemFatal("bind");

  //listen for fd_news
  // INSTEAD OF LISTENING need to get host id
  if(listen (fd_server, SOMAXCONN) == -1)
      SystemFatal("listen");

  // create epoll file Description
  epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
  if (epoll_fd == -1)
      SystemFatal("epoll_create");

  //add server socket to epoll event loop
  event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
  event.data.fd = fd_server;
  if(epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd_server, &event) == -1)
      SystemFatal("epoll_ctl");

  // Execute the epoll event loop
  while (1)
  {
    num_fds = epoll_wait (epoll_fd, events, EPOLL_QUEUE_LEN, -1);
    if (num_fds <0)
      SystemFatal("Error in epoll_wait");

    for(int i=0; i<num_fds; i++){
      // error conditions
      if(events[i].events & (EPOLLHUP | EPOLLERR))
      {
          fputs("epoll : EPOLLERR", stderr);
          close(events[i].data.fd);
          continue;
      }
      assert (events[i].events & EPOLLIN);


      //REMOVE NO SUCH THING fix for data
      if(events[i].data.fd == fd_server){
          fd_new = accept (fd_server, (struct sockaddr*) &remote_addr, &addr_size);
          if(fd_new == -1)
          {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("accept");
            }
            continue;
          }

          if(fcntl (fd_new, F_SETFL, O_NONBLOCK | fcntl (fd_new, F_GETFL, 0)) == -1)
              SystemFatal("fcntl");

          event.data.fd = fd_new;
          if(epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd_new, &event) == -1)
              SystemFatal ("epoll_ctl");

          continue;
      }
      //change to send data
      if(!ClearSocket(events[i].data.fd))
      {

        close(events[i].data.fd);
      }

    }
  }
  close(fd_server);
  exit(EXIT_SUCCESS);


}

void getForwardingInfo(){
  //where we will get all the forwarding info addresses and ports
  FILE *f  = fopen(FILEPATH,"r");
  int c,i=0,j=0;
  while((c=getc(f)) != EOF){
    //printf("%c",c);
    if(c == ',' || c == '\n' ){
      printf("%s\n",address_port[i]);
      i++;
      j=0;
    }else{
      address_port[i][j] = c;
      j++;
    }
  }
  printf("\n");
}

// can be removed
static int ClearSocket (int fd)
{
	int	n, bytes_to_read;
	char	*bp, buf[BUFLEN];

	while (1)
	{

		bp = buf;
		bytes_to_read = BUFLEN;
		while ((n = recv (fd, bp, bytes_to_read, 0)) < BUFLEN)
		{
			bp += n;
			bytes_to_read -= n;
		}
		printf ("sending:%s\n", buf);

		send (fd, buf, BUFLEN, 0);
		close (fd);
		return 1;
	}
	close(fd);
	return(0);

}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char* message)
{
    perror (message);
    exit (EXIT_FAILURE);
}

// close fd
void close_fd (int signo)
{
  close(fd_server);
	exit (EXIT_SUCCESS);
}
