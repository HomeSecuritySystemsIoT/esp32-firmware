/*
 * server_reacher.c
 *
 *  Created on: 26 kwi 2026
 *      Author: Dawid
 */
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <socket.h>
#include <stdlib.h>
#include <stdio.h>


int tcp_connect_server(const char *host, const char *port)
{
	struct addrinfo hint,*ret,*temp;
	memset(&hint,0,sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, port, &hint, &ret) != 0) {
		return -1;
	}
	temp = ret;
	int sfd;
	unsigned char * ip_temp;
	unsigned short* port_temp;
	while (temp) {
		printf("Attempting to create socket %s:%s\n",host,port);
		sfd = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
		if (sfd == -1) {
			puts("socket() failure");
			temp = temp->ai_next; 
			continue;
		}
		puts("socket() success");
		ip_temp=(unsigned char*)temp->ai_addr->sa_data+2;
		port_temp=(unsigned short*)temp->ai_addr->sa_data;
		printf("Attempting to connect to %hhu.%hhu.%hhu.%hhu:%hu\n",ip_temp[0],
		ip_temp[1],ip_temp[2],ip_temp[3],ntohs(*port_temp));
		if (connect(sfd, temp->ai_addr,temp->ai_addrlen) == -1) {
			printf("Connect failed. errno: %d (%s)\n", errno, strerror(errno));
			close(sfd);
		}
		else{
			puts("connect() success");
				//int optval = 1;
				//int err = setsockopt(sfd, SOL_SOCKET, 
				//	SO_REUSEADDR, &optval, sizeof(int));
				//if(err==-1){
				//	freeaddrinfo(ret);
				//	close(sfd);
				//	return -1;
				//}
			break;
		}
		temp = temp->ai_next;
	}
	if (temp == NULL) {
		freeaddrinfo(ret);
		return -1;
	}
	
	
	
	
	
	ip_temp=(unsigned char*)temp->ai_addr->sa_data+2;
		port_temp=(unsigned short*)temp->ai_addr->sa_data;
		printf("Connected to %hhu.%hhu.%hhu.%hhu:%hu\n",ip_temp[0],
		ip_temp[1],ip_temp[2],ip_temp[3],ntohs(*port_temp));
		
		freeaddrinfo(ret);
	
	return sfd;
}



int udp_connect_server(const char *host, const char *port)
{
    struct addrinfo hint, *ret, *temp;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;      
    hint.ai_socktype = SOCK_DGRAM;  

    if (getaddrinfo(host, port, &hint, &ret) != 0) {
        return -1; 
    }

    temp = ret;
    int sfd = -1;
	unsigned char * ip_temp;
	unsigned short* port_temp;
    while (temp) {
		printf("Attempting to create socket to %s:%s\n",host,port);
        sfd = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
        if (sfd == -1) {
			puts("socket() failure");
            temp = temp->ai_next;
            continue;
        }
        puts("socket() success");
		ip_temp=(unsigned char*)temp->ai_addr->sa_data+2;
		port_temp=(unsigned short*)temp->ai_addr->sa_data;
		printf("Attempting to connect to %hhu.%hhu.%hhu.%hhu:%hu\n",ip_temp[0],
		ip_temp[1],ip_temp[2],ip_temp[3],ntohs(*port_temp));
        if (connect(sfd, temp->ai_addr, temp->ai_addrlen) == -1) {
            close(sfd);
            puts("connect() fail");
            temp = temp->ai_next;
            continue;
        } else {
            puts("connect() success");
				//int optval = 1;
				//int err = setsockopt(sfd, SOL_SOCKET, 
				//	SO_REUSEADDR, &optval, sizeof(int));
				//if(err==-1){
				//	freeaddrinfo(ret);
				//	close(sfd);
				//	return -1;
				//}
				break;
        }
    }
   
    if (temp == NULL) {
		freeaddrinfo(ret);
		return -1;
	}
	ip_temp=(unsigned char*)temp->ai_addr->sa_data+2;
		port_temp=(unsigned short*)temp->ai_addr->sa_data;
		printf("Connected to %hhu.%hhu.%hhu.%hhu:%hu\n",ip_temp[0],
		ip_temp[1],ip_temp[2],ip_temp[3],ntohs(*port_temp));
	
	 freeaddrinfo(ret);
    return sfd; 
}

