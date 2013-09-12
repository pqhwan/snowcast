/*
 * =====================================================================================
 *
 *       Filename:  snowcast_control.c
 *
 *    Description:  the TCP client for snowcast
 *        Created:  09/06/2013 08:47:04 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT "8080" // the port client will be connecting to 
#define MAXDATASIZE 100 // max number of bytes we can get at once 


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
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}		/* -----  end of function get_in_addr  ----- */



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
	int
main ( int argc, char *argv[] )
{
	//set up variables
	struct addrinfo hints, *servinfo, *p;
	int status, sockfd;

	if(argc != 4){
		fprintf(stderr, "usage: snowcast_control servername serverport udpport");
		exit(1);
	}
	
	//set up hints
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//get address info
	if( (status = getaddrinfo(argv[1],argv[2],&hints, &servinfo)) != 0){
		fprintf(stderr,"client: getaddrinfo failed: %s",gai_strerror(status));
		exit(1);
	}

	//go through the linked list, connect to the first available socket
	for(p=servinfo; p!=NULL; p=p->ai_next){

		//try making a socket
		if( (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==-1){
			perror("socket could not be created");
			continue;
		}
		
		if( connect(sockfd, p->ai_addr,p->ai_addrlen) == -1 ) {
			perror("connect() failed:");
			continue;
		}

		break;
	}
	
	//Was connection not available?
	if(p==NULL){
		perror("client could not be connected");
		exit(1);
	}

	//Who is that?
	char buf[INET6_ADDRSTRLEN];
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), buf, sizeof buf);
	printf("connected to server: %s\n", buf);

	//Let's say hello
	send(sockfd, "h4444", 5, 0);
	freeaddrinfo(servinfo);

	//Anybody there?
	int rec_bytes;
	char buff[500];
	if((rec_bytes = recv(sockfd,buff,500,0) )== -1) {
		perror("recv() failed");
		exit(1);
	}

	//What'd she say?
	buff[rec_bytes] = '\0';
	printf("recieved: %s\n",buff);

	//I might want to stop talking to her
	int quit=0;

	//for talking to the user 
	char command[1024];
	ssize_t command_bytes;
	size_t max = sizeof command;
	int i, not_int;

	//for talking to the server
	char response[512];
	ssize_t response_bytes;
	size_t response_size = sizeof response;
	int maxstation = 10;
	int newstation;
	
	//for conversating with 2 people at the same time
	struct timeval tv;
	fd_set readfds, masterfds;
	int tv_usec = 1;
	int tv_sec = 1;
	int maxfd = sockfd;
	int change;
	FD_ZERO(&masterfds);
	FD_SET(0, &masterfds);
	FD_SET(sockfd, &masterfds);

	while(!quit){
		//tidy myself for another select()
		printf("s\n");
		readfds= masterfds;
		tv.tv_usec = tv_usec;
		tv.tv_sec = tv_sec;

		//Did somebody say something?
		if((change = select(maxfd+1, &readfds, NULL, NULL, &tv)) == -1){
			perror("error on select()");
			close(sockfd);
			exit(1);
		}
	
		//was it the user?
		if(FD_ISSET(0, &readfds)){
			if( (command_bytes = read(0,command,max)) == -1){
				perror("error on read()");
				exit(1);
			}
			if(command_bytes == 1){
				continue;
			}
			command[command_bytes] = '\0';

			//is this an integer?
			not_int = 0;
			for(i=0;i<command_bytes-1;i=i+1){
				if(!isdigit(command[i])){
					not_int = 1;
					break;
				}
			}

			if(!not_int){
				if( (newstation = atoi(command)) > maxstation){
					fprintf(stderr, "only stations from 0-%d are available\n",maxstation);
					continue;
				}	
				printf("requesting setstation to station no. %d\n", newstation);
				//TODO send setstation
				//no need to receive here though
			}else if(command_bytes == 2){
				if(command[0] == 'q')
					quit = 1;
			//	if(command[0] == 'h')
					//printhelp();
			}	
		}

		//was it the server?
		if(FD_ISSET(sockfd, &readfds)){
			if( (response_bytes = recv(sockfd, response, response_size, 0)) == -1){
				perror("recv()");
				continue;
			}
			response[response_bytes] = '\0';
			printf("server response: %s\n", response);	
		}
	}

	close(sockfd);
	
	return 0;
}				/* ----------  end of function main  ---------- */
