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


#define MAXDATA 100


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
		return & ((( struct sockaddr_in *)sa) -> sin_addr);
	}
	return & ((( struct sockaddr_in6 *)sa) -> sin6_addr);
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
	//for creating a passive socket 
	struct addrinfo hints, *servinfo, *p;
	int status, sockfd;

	if(argc != 2){
		fprintf(stderr, "usage: snowcast_listener udpport\n");
		exit(1);
	}


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	
	if ( (status = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0){
		fprintf(stderr, "getaddrinfo : %s", gai_strerror(status));
		exit(1);
	}

	for(p=servinfo;p!=NULL;p=p->ai_next){
		if( (sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
			perror("socket could not be created");
			continue;
		}

		if( bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			close(sockfd);
			perror("bind failed");
			continue;
		}
		break;
	}

	if(p == NULL){
		perror("port not available");
		exit(1);
	}

	printf("socket setup successful!\n");

	freeaddrinfo(servinfo);

	//for receiving packets
	struct sockaddr_storage their_addr;
	ssize_t bytes_received;
	socklen_t addr_len = sizeof their_addr;
	char data[MAXDATA];
	char addrbuf[INET6_ADDRSTRLEN];
	
	while(1){
		//let's listen
		if((bytes_received = recvfrom(sockfd, data, MAXDATA-1, 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1){
			perror("recvfrom");
			exit(1);
		}

		//who sent this?
		printf("%zd bytes packet sent from: %s\n",bytes_received,
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *) &their_addr),
			addrbuf, sizeof addrbuf));

		//what's the message?
		data[bytes_received] = '\0';
		printf("packet contents: \n%s\n", data);
	}

    	close(sockfd);

    	return 0;
}

				/* ----------  end of function main  ---------- */
