/*
 * =====================================================================================
 *
 *       Filename:  snowcast_listener.c
 *
 *    Description:  the UDP client for snowcast
 *
 *        Version:  1.0
 *        Created:  09/09/2013 11:31:00 PM
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "serial.h"

#define MAXDATA 100


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
	int
main ( int argc, char *argv[] )
{
	//<-SET UP UDP SOCKET->
	struct addrinfo hints;
	int sockfd;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	sockfd = socket_setup(hints, argv[1], NULL); 
	printf("socket on port %d setup successful!\n", atoi(argv[1]));

	//for receiving packets
	struct sockaddr_storage sender_addr;
	ssize_t bytes_received;
	socklen_t addr_len = sizeof sender_addr;
	char data[MAXDATA];

	char addrbuf[INET6_ADDRSTRLEN];
	
	while(1){
		//let's listen
		if((bytes_received = recvfrom(sockfd, data, MAXDATA-1, 0,
			(struct sockaddr *)&sender_addr, &addr_len)) == -1){
			perror("recvfrom");
			exit(1);
		}

		//who sent this?
		printf("%zd bytes packet sent from: %s\n",bytes_received,
		inet_ntop(sender_addr.ss_family,
			get_in_addr((struct sockaddr *) &sender_addr),
			addrbuf, sizeof addrbuf));

		//what's the message?
		data[bytes_received] = '\0';
		printf("packet contents: \n%s\n", data);
	}

    	close(sockfd);

    	return 0;
}

				/* ----------  end of function main  ---------- */
