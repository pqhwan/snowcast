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
#include <sys/select.h>
#include <ctype.h>
#include "serial.h"
#define PORT "8080" // the port client will be connecting to 
#define MAXREPLYSIZE 512 // max number of bytes we can get at once 
#define COMMANDSIZE sizeof(uint8_t) + sizeof(uint16_t)
#define MAXREADSIZE 128


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
	int
main ( int argc, char *argv[] )
{
	struct addrinfo hints;
	int sockfd, sen_bytes, rec_bytes, quit = 0;
	unsigned char command[COMMANDSIZE], reply[MAXREPLYSIZE];
	char ucommand[MAXREADSIZE];
	uint8_t replyType;
	uint16_t numStations;
	
	if(argc != 4){
		fprintf(stderr, "usage: snowcast_control servername serverport udpport\n");
		exit(1);
	}

	//connect to server	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	sockfd = socket_setup(hints, argv[2], argv[1]);
	printf("connected to %s:%d\n", argv[1], atoi(argv[2]));

	//say hello
	pack(command, "ch", (uint8_t)0, (uint16_t)atoi(argv[3]));
	if((sen_bytes = send(sockfd, command, sizeof command, 0)) == -1){
		perror("send() failed");
		exit(1);
	}
	
	//Anybody there?
	if((rec_bytes = recv(sockfd, reply, sizeof reply,0) )== -1) {
		perror("recv() failed");
		exit(1);
	}

	//What'd she say?
	//TODO error checking??
	unpack(reply, "ch", &replyType, &numStations);
	printf("-bytes_receved: %d\n-replyType: %hd\n-numstations: %d\n",rec_bytes,replyType,numStations);

	//for talking to the user 
	ssize_t command_bytes;
	size_t max = sizeof command;
	int i, not_int;

	//for talking to the server
	ssize_t reply_bytes;
	size_t reply_size= sizeof reply;
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
			if( (command_bytes = read(0,ucommand,max)) == -1){
				perror("error on read()");
				exit(1);
			}
			if(command_bytes == 1){
				continue;
			}
			ucommand[command_bytes] = '\0';

			//is this an integer?
			not_int = 0;
			for(i=0;i<command_bytes-1;i=i+1){
				if(!isdigit(command[i])){
					not_int = 1;
					break;
				}
			}

			if(!not_int){
				if( (newstation = atoi(ucommand)) > maxstation){
					fprintf(stderr, "only stations from 0-%d are available\n",maxstation);
					continue;
				}	
				printf("requesting setstation to station no. %d\n", newstation);
				//TODO send setstation
				//no need to receive here though
			}else if(command_bytes == 2){
				if(ucommand[0] == 'q')
					quit = 1;
			//	if(command[0] == 'h')
					//printhelp();
			}	
		}

		//was it the server?
		//expect announce or invalid command: 8, 8 , char *
		if(FD_ISSET(sockfd, &readfds)){
			if( (reply_bytes = recv(sockfd,reply,reply_size, 0)) == -1){
				perror("recv()");
				continue;
			}
			reply[reply_bytes] = '\0';
			printf("server reply: %s\n", reply);
		}
	}

	close(sockfd);
	
	return 0;
}				/* ----------  end of function main  ---------- */
