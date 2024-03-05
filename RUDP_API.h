#ifndef RUDP_H
#define RUDP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define FILE_SIZE 2*1048576 
#define TRANSMISSION_TRIES 4
#define BUFFER_SIZE 32741

int rudp_socket(int type, int port, char *ip);
int rudp_send(int client_socket, FILE *file, int port, char *ip);
int rudp_recv(int sockfd, char *buffer, int len, struct sockaddr_in *src_addr);
int rudp_close(int sockfd);
uint16_t calculate_checksum(char *data, size_t size);

#endif /* RUDP_H */
