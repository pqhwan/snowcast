/*
 * =====================================================================================
 *
 *       Filename:  serial.c
 *
 *    Description:  pack and unpack data
 *
 *        Version:  1.0
 *        Created:  09/12/2013 11:01:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */


#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "serial.h"






/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  socket_setup
 *  Description:  
 * =====================================================================================
 */
	int
socket_setup (struct addrinfo hints, char* port, char *host)
{
	struct addrinfo *p, *addr;
	int status, sockfd, yes=1;

	if((status = getaddrinfo(host, port, &hints, &addr)) != 0){
		fprintf(stderr, "getaddrinfo() - %s \n", gai_strerror(status));
	}

	for(p = addr; p!=NULL; p=p->ai_next){
		if((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			perror("socket()");
			continue;
		}

		if(hints.ai_flags==AI_PASSIVE){
			if(bind(sockfd, p->ai_addr, p->ai_addrlen) != 0){
				perror("bind()");
				close(sockfd);
				continue;
			}

			if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
				perror("setsockopt()");
				exit(1);
			}
		} else{
			if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
				perror("connect()");
				continue;
			}
		}

		
		break;
	}

	if(p==NULL){
		perror("no connection found");
		exit(1);
	}
	freeaddrinfo(addr);
	return sockfd;
}		/* -----  end of function socket_setup  ----- */

	void *
get_in_addr (struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void packi16(unsigned char *buf, unsigned int i){
	//store the first byte
	*buf++ = i >> 8;
	//store the next byte
	*buf++ = i;
}

unsigned int unpacki16(unsigned char *buf){
	return (buf[0]<<8) | buf[1];
}


//returns amount packed in bytes
unsigned int pack(unsigned char *buf, char *format, ...){
	va_list ap;

	//unsigned 8 bit
	unsigned char c;

	//unsigned 16 bit
	unsigned int h;

	//string
	char *s;
	unsigned int len;

	unsigned int size = 0;

	va_start(ap, format);

	for(; *format != '\0'; format++) {
		switch(*format) {
		case 'c': // 8-bit unsigned
			size += 1;
			c = (unsigned char)va_arg(ap, unsigned int); // promoted
			*buf++ = c;
			break;

		case 'h': // 16-bit unsigned
			size += 2;
			h = va_arg(ap, unsigned int);
			packi16(buf, h);
			buf += 2;
			break;

		case 's': // string
			s = va_arg(ap, char*);
			len = strlen(s);
			size += len + 2;
			packi16(buf, len);
			buf += 2;
			memcpy(buf, s, len);
			buf += len;
			break;
		}
	}

	va_end(ap);

	return size;
}

void unpack(unsigned char *buf, char *format, ...)
{
	va_list ap;
	uint8_t *c;
	uint16_t *h;
	char *s;
	int32_t len, count, maxstrlen = 0;

	va_start(ap, format);

	for(; *format != '\0'; format++){
		switch(*format){
		case 'c':
			c = va_arg(ap, uint8_t*);
			*c = *buf++;
			break;
		case 'h':
			h = va_arg(ap, uint16_t*);
			*h = unpacki16(buf);
			buf += 2;
			break;
		case 's':
			s = va_arg(ap, char*);
			len = unpacki16(buf);
			buf +=2;
			if(maxstrlen>0 && len > maxstrlen) count = maxstrlen-1;
			else count = len;
			memcpy(s,buf,count);
			s[count] = '\0';
			buf += len;
			break;
		}

	}
}

