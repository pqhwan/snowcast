
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
#include "serial.h"


#define BACKLOG 10
#define STATIONNUM 10

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
	struct station_info *si_next;
	pthread_t thread_id;
};

int udp_sourcefd;
int tcp_sourcefd;
int client_count=0;



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  clist_display
 *  Description:  
 * =====================================================================================
 */
	void
clist_display (struct client_info **list_head)
{
	struct client_info *c;
	for(c=*list_head;c!=NULL;c=c->ci_next){
		printf("%d -> ", c->ci_fd);
	}
	printf("end\n");
}		/* -----  end of function clist_display  ----- */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  list_add
 *  Description:  
 * =====================================================================================
 */
 
	void	
clist_add (struct client_info *element, struct client_info **list_head)
{
	if(*list_head == NULL){
		printf("list empty: adding %d to list\n",element->ci_fd);
		*list_head = element;
		return;
	}
	struct client_info *c;
	for(c=*list_head;c!=NULL;c=c->ci_next){
		printf("iterating: client with fd %d\n",c->ci_fd);
		if(!c->ci_next){
			printf("reached end of list\n");
			c->ci_next = element;
			return;
		}

	}
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  list_remove
 *	   Description: 
 * =====================================================================================
 */
	int
clist_remove (struct client_info *element, struct client_info **list_head)
{
	if(*list_head == NULL){
		return -1;
	} 

	struct client_info *c;
	struct client_info *prev = NULL;
	for(c=*list_head;c!=NULL;prev=c,c=c->ci_next){	
		if(c->ci_fd == element->ci_fd){
			printf("c->ci_fd and element->ci_fd are %d\n",c->ci_fd);
			if(!prev){
				*list_head = c->ci_next;
				break;
			} else {
				prev->ci_next = c->ci_next;
				break;
			}
		}
	}
	return 0;
}

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
		printf("%s\n", sinfo->songname);
		FILE *file = fopen(sinfo->songname,"r");
		if(file == NULL){
			printf("failed to open file \n");
			exit(1);
		}
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
 *         Name:  spawn_stream_thread
 *  Description:  
m* =====================================================================================
 */
	pthread_t
spawn_stream_thread (char *song, struct station_info **station)
{
	*station = (struct station_info *) malloc(sizeof(struct station_info));
	(*station)->songname = song;
	(*station)->client_list = NULL;
	(*station)->si_next = NULL;
	pthread_t thread_id;
	pthread_create(&thread_id, 0, (void *)&stream, *station);
	(*station)->thread_id = thread_id;
	return thread_id;
}		/* -----  end of function spawn_stream_thread  ----- */



	int
main ( int argc, char *argv[] )
{	

	if(argc !=2){
		fprintf(stderr, "usage: snowcast_server tcpport\n");
		exit(1);
	}

	/*<-SET UP PASSSIVE TCP SOCKET->*/
	struct addrinfo hints;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 
	tcp_sourcefd = socket_setup(hints,argv[1], NULL);
	printf("tcp socket (fd %d) on port %d setup successful!\n",tcp_sourcefd, atoi(argv[1]));

	/*<-SET UP PASSIVE UDP SOCKET->*/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; 
	udp_sourcefd = socket_setup(hints, argv[1], NULL);
	printf("udp socket (fd %d) on port %d setup successful!\n", udp_sourcefd, atoi(argv[1]));

	/*<-SET UP STREAMING THREADS->*/
	int i;
	for(i=0;i<STATIONNUM;i++){

	}
	struct station_info *station_0;
	spawn_stream_thread("test.text", &station_0);
	struct station_info *station_1;
	spawn_stream_thread("a.text", &station_1);
	station_0->si_next = station_1;

	/*<-MISC DECLARATION->*/
	struct station_info *s;
	struct client_info *pending_clients = NULL;

	
	/*<-INITIATE SERVICE->*/ 
	//for select()
	fd_set masterfds, readfds;
	FD_ZERO(&masterfds);
	FD_SET(0, &masterfds);
	FD_SET(tcp_sourcefd, &masterfds);
	struct timeval tv, tvcop;
	tv.tv_sec = 1;

	int maxfd = tcp_sourcefd;

	//for accepting new clients
	struct sockaddr_storage their_addr;
	socklen_t ss_size = sizeof their_addr;
	char addr_str[INET6_ADDRSTRLEN];
	if(listen(tcp_sourcefd, 10) == -1){
		perror("program: error on listen");
		exit(1);
	}

	//for talking to clients
	uint16_t numStations = 1, udpPort;
	unsigned char request[512];
	int request_bytes, response_bytes, packetSize;
	struct client_info *c;
	uint8_t commandType;

	//for talking to the user
	char command[512];
	ssize_t command_bytes;

	while(1){
		/* <-CLEAN UP FOR ANOTHER SELECT-> */
		printf("waiting...\n");
		readfds = masterfds;
		tvcop = tv;
	

		/*<-SELECT-> */
		if(select(maxfd+1, &readfds, NULL, NULL, &tvcop) == -1){
			perror("program: error on select()");
			close(tcp_sourcefd);
			//close(newfd);
			exit(1);
		}

		/*<-ACTIVE TCP SOCKET CHECK for ACTIVE CLIENTS-> */
		//expects setStation: 8 +16

		for(s=station_0;s!=NULL;s=s->si_next){
			printf("1\n");
		for(c=s->client_list;c!=NULL;c=c->ci_next){
			//did you say something, i?
			printf("checking active client %d\n",c->ci_fd);
			if(FD_ISSET(c->ci_fd, &readfds)){
				printf("new message from active client %d\n", c->ci_fd);
				if((request_bytes = recv(c->ci_fd, request, sizeof request, 0)) == -1){
					perror("recv()");
					continue;
				}

				if(!request_bytes){
					printf("client quits\n");
					//TODO get rid of the client from the active list
					//TODO how to get the next highest fd number?
					if(c->ci_fd == maxfd) maxfd = maxfd-1;
					FD_CLR(c->ci_fd, &masterfds);
					close(c->ci_fd);
					continue; 
				}

				unpack(request, "ch", &commandType, &udpPort);
				printf("bytes_received: %d \n commandType: %hd\n udpPort: %d\n", request_bytes, commandType, udpPort);
			}

		}
		}
	
		/*<-PASSIVE TCP: New Client connect()-> */	
		if (FD_ISSET(tcp_sourcefd, &readfds)){
			client_count++;
			printf("new client!\n");

			int clientfd = accept(tcp_sourcefd, (struct sockaddr *)&their_addr, &ss_size);
			if ( clientfd == -1 ) {
				perror("program: error on accept()\n");
				close(tcp_sourcefd);
				exit(1);
			}

			//announce new connection
			inet_ntop(their_addr.ss_family,
				get_in_addr( (struct sockaddr *)&their_addr),
				addr_str, sizeof addr_str);
			printf("connection accepted from: %s\n", addr_str);

			//make client_info struct 
			struct client_info *newclient = (struct client_info *) malloc(sizeof(struct client_info));
			newclient->ci_family = their_addr.ss_family;
			newclient->ci_addr = addr_str;
			newclient->ci_next = NULL;
			newclient->ci_fd = clientfd;

			//add client to the pending list
			printf("new connection fd: %d\n",clientfd);
			clist_add(newclient, &pending_clients);
			clist_display(&pending_clients);
			FD_SET(clientfd, &masterfds);
			if(maxfd<clientfd)maxfd= clientfd;
		}

	

		/*<-ACTIVE TCP SOCKET CHECK for PENDING CLIENTS-> */
		//expects hello: 8 + 16
		for(c=pending_clients;c!= NULL;c=c->ci_next){
			printf("checking pending client %d\n", c->ci_fd);
			if(FD_ISSET(c->ci_fd, &readfds)){
				printf("new message from pending client %d\n", c->ci_fd);
				if((request_bytes = recv(c->ci_fd, request, sizeof request, 0)) == -1){
					perror("recv()");
					continue;
				}
				if(!request_bytes){
					printf("client quits\n");
					//TODO get rid of the client from the pending list
					//-----unfortunately, this client has not been able to provide us with his
					//-----UDP port. He passed away too early.
					//TODO how to get the next highest fd number?
					if(c->ci_fd == maxfd) maxfd = maxfd-1;
					FD_CLR(c->ci_fd, &masterfds);
					close(c->ci_fd);
					continue; 
				}	

				unpack(request, "ch", &commandType, &udpPort);
				printf("bytes_received: %d \n commandType: %hd\n udpPort: %d\n", request_bytes, commandType, udpPort);
 
				//was that a hello?
				if(commandType == 0){
					printf("hello received\n");

					//send back welcome
					int welcomesize = sizeof(uint8_t) + sizeof(uint16_t);
					unsigned char response[welcomesize];
					packetSize = pack(response, "ch", (uint8_t)0, (uint16_t)numStations);

					if( (response_bytes = send(c->ci_fd, response, sizeof response, 0)) == -1){
						perror("send() failed");
						exit(1);
					}
					if(packetSize != response_bytes){
						printf("%d bytes have been packed, but %d bytes have been sent\n", packetSize, response_bytes);
						exit(1);
					}

					//complete client's struct info
					c->ci_udp = udpPort;
					struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
					addr->sin_family = c->ci_family;
					addr->sin_port = htons(c->ci_udp);
					addr->sin_addr = *((struct in_addr *) gethostbyname(c->ci_addr)->h_addr);
					c->ci_udp_addr = (struct sockaddr *) addr;

					//add client to default stations client list
					printf("i->ci_fd: %d\n", c->ci_fd);
					if(client_count==1)clist_add(c,&station_0->client_list);
					if(client_count==2)clist_add(c,&station_1->client_list);
					clist_remove(c,&pending_clients);
					printf("pending list: ");
					clist_display(&pending_clients);
					printf("active list: ");
					clist_display(&station_0->client_list);
				} else {
					//invalid command
					clist_remove(c, &pending_clients);
					FD_CLR(c->ci_fd, &masterfds);
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










