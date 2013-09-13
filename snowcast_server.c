/*
 * =====================================================================================
 *
 *       Filename:  snowcast_server.c
 *
 *    Description:  The server for snowcast 
 *
 *        Version:  1.0
 *        Created:  09/04/2013 08:20:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>


#define BACKLOG 10

struct client_info{
	//for talking to the TCP client
	int ci_fd;
	//for constructing the UDP address
	unsigned short ci_udp;
	short ci_family;
	char *ci_addr;
	struct sockaddr *ci_udp_addr;
	//linked list component
	struct client_info *ci_next;
};

struct station_info{
	char *songname;
	struct client_info *client_list;
};

int udp_sourcefd;
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  stream
 *  Description:  function passed to the station thread.
 *  Parameters: name of the file to stream, linkedlist of clients to stream to
 * =====================================================================================
 */
	void
stream (struct station_info *sinfo)
{
	ssize_t bytes_sent;
	char line[512];
	struct client_info *c;
	socklen_t addr_len = sizeof(struct sockaddr);
	while(1){
		FILE *file = fopen(sinfo->songname,"r");
		printf("opened file %s\n",sinfo->songname);

		memset(line,0,sizeof line);

		while(fgets(line,sizeof line, file) != NULL){
			for(c=sinfo->client_list;c!=NULL;c=c->ci_next){
				bytes_sent =sendto(udp_sourcefd, line, sizeof line, 0,
					c->ci_udp_addr, addr_len);
				if(bytes_sent == -1){
					perror("sendto()");
					exit(1);
				}
				printf("sent  %zd bytes\n", bytes_sent);
			}
			sleep(1);
		}
		fclose(file);
	}
}		/* -----  end of function stream  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  get_in_addr
 *  Description:  
 * =====================================================================================
 */
	void *
get_in_addr (struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}		/* -----  end of function get_in_addr  ----- */



	int
main ( int argc, char *argv[] )
{

	if(argc !=2){
		fprintf(stderr, "usage: snowcast_server tcpport\n");
		exit(1);
	}

	/*<-SET UP PASSSIVE TCP SOCKET->*/
	int status, sockfd, yes=1;
	struct addrinfo hints, *passiveaddr, *p, *udpaddr;
	struct sockaddr_storage their_addr;
	socklen_t ss_size;

	memset(&hints, 0,sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 

	//getaddrinfo for passive socket (port 8080)	
	if ( (status = getaddrinfo(NULL,argv[1], &hints, &passiveaddr) != 0)){
		fprintf(stderr, "program: getaddrinfo() - %s\n", gai_strerror(status));
	}

	for (p = passiveaddr; p!=NULL; p = p->ai_next){
		//make a socket
		if ( (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==-1 ) {
			perror("program: error on socket()");
			continue;
		}

		//makes the port instantly reusbale after server is closed
 		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
	            perror("setsockopt");
       		    exit(1);
	        }

		//bind the socket to the sockaddr inside ai_addr
		if( bind(sockfd, p->ai_addr, p->ai_addrlen) !=0){
			perror("program: error on bind()");
			close(sockfd);
			continue;
		}
		break;
	}

	//no suitable port has been found
	if ( p == NULL ) {
		perror("no connection found");
		exit(1);
	}	

	//No need for this: Passive socket all set up. 
	freeaddrinfo(passiveaddr);
	printf("tcp socket setup successful!\n");

	/*<-SET UP PASSIVE UDP SOCKET->*/
	int udpfd;
	memset(&hints, 0, sizeof hints);
	memset(p,0,sizeof p);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ( (status = getaddrinfo(NULL, argv[1], &hints, &udpaddr)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
	}

	for(p=udpaddr;p!=NULL;p=p->ai_next){
		if( (udpfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			perror("socket()");
			continue;
		}

		if(bind(udpfd, p->ai_addr, p->ai_addrlen) == -1){
			close(udpfd);
			close(sockfd);
			perror("bind()");
			continue;
		}
		break;
	}

	if(p == NULL) {
		fprintf(stderr, "no suitable address found\n");
		close(udpfd);
		close(sockfd);
		exit(1);
	}

	freeaddrinfo(udpaddr);
	printf("udp socket setup successful!\n");
	udp_sourcefd = udpfd;


	/*<-SET UP STREAMING THREAD->*/
	//struct client_info *station_0_clients = NULL;
	struct station_info *station_0_info = malloc(sizeof(station_0_info));
	station_0_info->songname = "test.text";
	station_0_info->client_list = NULL; 
	pthread_t thread_id;
	pthread_create(&thread_id, 0, (void *)&stream, station_0_info);


	/*<-INITIATE SERVICE->*/ 
	//for select()
	struct timeval tv;
	int tv_usec = 1;
	int tv_sec = 1;
	fd_set masterfds;
	FD_ZERO(&masterfds);
	FD_SET(0, &masterfds);
	FD_SET(sockfd, &masterfds);
	int maxfd = sockfd;
	fd_set readfds;

	//for accepting new clients
	ss_size = sizeof their_addr;
	char addr_str[INET6_ADDRSTRLEN];
	if(listen(sockfd, 10) == -1){
		perror("program: error on listen");
		exit(1);
	}
	
	//for talking to clients
	char request[512];
	int request_bytes;
	struct client_info *pending_clients = NULL;
	struct client_info *i;

	//for talking to the user
	char command[512];
	ssize_t command_bytes;

	while(1){
		/* <-CLEAN UP FOR ANOTHER SELECT-> */
		printf("..\n");
		readfds = masterfds;
		tv.tv_usec = tv_usec;
		tv.tv_sec = tv_sec;
	

		/*<-SELECT-> */
		if(select(maxfd+1, &readfds, NULL, NULL, &tv) == -1){
			perror("program: error on select()");
			close(sockfd);
			//TODO for now, 1 client
			//close(newfd);
			exit(1);
		}
	
		/*<-PASSIVE TCP-> */	
		if (FD_ISSET(sockfd, &readfds)){
			printf("new client!\n");
			int clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &ss_size);
			if ( clientfd == -1 ) {
				perror("program: error on accept()\n");
				close(sockfd);
				exit(1);
			}

			//let's tell the user who connected
			inet_ntop(their_addr.ss_family,
				get_in_addr( (struct sockaddr *)&their_addr),
				addr_str, sizeof addr_str);
			printf("connection accepted from: %s\n", addr_str);


			//add client to the pending list
			if(pending_clients == NULL){
				printf("this is the first client\n");
				struct client_info *newclient = malloc(sizeof(struct client_info));
				newclient->ci_family = their_addr.ss_family;
				newclient->ci_addr = addr_str;
				newclient->ci_next = NULL;
				newclient->ci_fd = clientfd;
				pending_clients = newclient;
			} else {
				printf("we've had client(s) before\n");
				//add this client at the end of the linkedlist
				struct client_info *i;
				for(i=pending_clients;i!=NULL;i=i->ci_next){
					

				}

			}
			FD_SET(clientfd, &masterfds);
			maxfd= clientfd;
			//hi client!
			send(clientfd, "welcome!", 8, 0);
		}

		/*<-ACTIVE TCP SOCKET CHECK for ACTIVE CLIENTS-> */
		for(i=station_0_info->client_list;i!=NULL;i=i->ci_next){
			//did you say something, i?
			if(FD_ISSET(i->ci_fd, &readfds)){
				printf("new message from active client %d\n", i->ci_fd);
				if((request_bytes = recv(i->ci_fd, request, sizeof request, 0)) == -1){
					perror("recv()");
					continue;
				}
				if(!request_bytes){
					printf("client quits\n");
					//TODO get rid of the client from the active list
					//TODO how to get the next highest fd number?
					if(i->ci_fd == maxfd) maxfd = maxfd-1;
					FD_CLR(i->ci_fd, &masterfds);
					close(i->ci_fd);
					continue; 
				}

				request[request_bytes] = '\0';
				printf("client command: %s \n", request);

			}

		}

		/*<-ACTIVE TCP SOCKET CHECK for PENDING CLIENTS-> */
		for(i=pending_clients;i!= NULL;i=i->ci_next){
			if(FD_ISSET(i->ci_fd, &readfds)){
				printf("new message from pending client %d\n", i->ci_fd);
				if((request_bytes = recv(i->ci_fd, request, sizeof request, 0)) == -1){
					perror("recv()");
					continue;
				}
				if(!request_bytes){
					printf("client quits\n");
					//TODO get rid of the client from the pending list
					//-----unfortunately, this client has not been able to provide us with his
					//-----UDP port. He passed away too early.
					//TODO how to get the next highest fd number?
					if(i->ci_fd == maxfd) maxfd = maxfd-1;
					FD_CLR(i->ci_fd, &masterfds);
					close(i->ci_fd);
					continue; 
				}

				request[request_bytes] = '\0';
				printf("client command: %s \n", request);

				//was that a hello?
				if(request[0] == 'h'){
					char *portaddr = request + 1;
					//put in udp port to the struct
					i->ci_udp = atoi(portaddr);
					struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
					addr->sin_family = i->ci_family;
					addr->sin_port = htons(i->ci_udp);
					addr->sin_addr = *((struct in_addr *) gethostbyname(i->ci_addr)->h_addr);
					i->ci_udp_addr = (struct sockaddr *) addr;
					
					//add him to the active linkedlist to start streaming
					if(station_0_info->client_list == NULL){
						i->ci_next = NULL;
						station_0_info->client_list = i;
						if(i == NULL){
							printf("i is null\n");
						}
						printf("Added first client to station 0!\n");
						//TODO remove this client from the pending list
						//temporary measure
						pending_clients = i->ci_next;
					} else {
						//TODO add this client at the end of the linked list
					}
				}else{
					//whatever this command is, it cannot come before a hello
					//this is where we send back an 'invalid command'
				}
			}
		}


		/*<-STDIN CHECK-> */	
		if(FD_ISSET(0, &readfds)){
			printf("new command!\n");
			command_bytes = read(0, command, sizeof command);
			if(command_bytes == -1){
				perror("read()");
				exit(1);
			}
			command[command_bytes] = '\0';
			printf("server admin command: %s\n", command);	
		}
	}
	
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */ 










