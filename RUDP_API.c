
#include <sys/time.h>
#include <errno.h>
#include <strings.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include "RUDP_API.h"

#define FILE_SIZE 2*1048576

#define TRANSMITION_TRIES 4


int rudp_socket(int type, int port, char *ip) {//type == 1 ->sender || type == 2 -> reciever
if(type == 1){//SENDER
    struct timeval tv = { 2, 0 };
    // Create a UDP socket
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0) )< 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in reciever_address;
    socklen_t length = sizeof(reciever_address);
    memset(&reciever_address,0,sizeof(reciever_address));//update
    reciever_address.sin_family = AF_INET;
    reciever_address.sin_port=htons(port);

    int rval = inet_pton(AF_INET,(const char*)ip,&reciever_address.sin_addr);
    if(rval <= 0){
        printf("function inet_pton() failed \n");
        return -1;
    }
 
    char buffer[3] = {0};
    printf("Sending SYN from sender.\n");
    


        if ((setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) )== -1)
    {
        perror("setsockopt(2)");
        close(sockfd);
        return -1;
    }

    int i = sendto(sockfd, "SYN", 3, 0, (struct sockaddr *)&reciever_address, length);
    if(i < 0){
        int c = rudp_close(sockfd);
        return -1;}

    // Wait for ACK from reciever
    recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *)&reciever_address, &length);
    if (strcmp(buffer, "ACK") == 0) {
        printf("Received ACK from reciever. Sending ACK...\n");
        printf("cilent_connected to server \n");
        sendto(sockfd, "ACK", strlen("ACK"), 0,(struct sockaddr *)&reciever_address, length);
    } else {
        printf("ACK not received. Connection failed.\n");
        perror("failed to establish connection");
        return -1;
    }

    return sockfd;
}
else if (type == 2)//RECIEVER
{
    struct sockaddr_in server_addr;

    char buffer[3] = {0};  

    // The  memset()  function  fills  the  first  n  bytes of the memory area
    //       pointed to by s with the constant byte c.
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port); // short, network byte order


    // Create a UDP socket
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("Couldn't create a socket : %d\n",errno);
        perror("socket creation failed");
        return -1;
    }

    /*When a socket is created with socket(2), it exists in a name space (ad‐
       dress family) but has no address assigned to it.   bind()  assigns  the
       address  specified  by  addr  to the socket referred to by the file de‐
       scriptor sockfd.  addrlen specifies the size, in bytes, of the  address
       structure  pointed to by addr.  Traditionally, this operation is called
       “assigning a name to a socket”.*/
    // connect the server to a port which can read and write on.
    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        printf("Bind failed with error code : %d" , errno);
        return -1;
    }

    socklen_t length = sizeof(server_addr);
    // Wait for ACK from reciever
    recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *)&server_addr, &length);
    if (strcmp(buffer, "SYN") == 0) {
        printf("Connection request received, sending ACK.\n");
        sendto(sockfd, "ACK", strlen("ACK"), 0,(struct sockaddr *)&server_addr, length);
        recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *)&server_addr, &length);
        if(strcmp(buffer, "ACK") == 0){
            printf("Sender connected, beginning to receive file...");
        }
        
    } else {
        printf("ACK not received from sender. Connection failed.\n");
        perror("failed to establish connection");
        return -1;
    }

    return sockfd; // return the socket that we create.

} else {

    perror("wrong type input in rudp_socket");
    return -1;
}}

int rudp_send(int client_socket, FILE *file, int port, char *ip) {
    char buffer[BUFFER_SIZE];
    int ack = 0;
    int retries = 0;
    int total_bytes_sent = 0;  // Variable to keep track of total bytes sent

    struct sockaddr_in dest_addr;
    socklen_t length = sizeof(dest_addr);
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &dest_addr.sin_addr);

    while (ack == 0 && retries < TRANSMITION_TRIES) {//while (ack == 0 && retries < TRANSMITION_TRIES) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
        if (bytes_read == 0) {
            if (feof(file)) {
                printf("End of file reached.\n");
            } else {
                perror("Error reading from file");
            }
            return total_bytes_sent;  // Return total bytes sent so far
        }

        ssize_t bytes_sent = sendto(client_socket, buffer, bytes_read, 0, (struct sockaddr *)&dest_addr, length);
        if (bytes_sent < 0 && retries >= TRANSMISSION_TRIES) {
            perror("Sendto failed");
            exit(EXIT_FAILURE);
            //return total_bytes_sent;  // Return total bytes sent so far
        }

        total_bytes_sent += bytes_sent;  // Update total bytes sent

        // Receive ACK from sender
        //socklen_t dest_addr_len = sizeof(dest_addr);
        char ack_buffer[3] = {0};
        recvfrom(client_socket, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&dest_addr, &length);

        if (strcmp(ack_buffer, "ACK") == 0) {
            printf("Received ACK from sender. Sending ACK...\n");
            printf("Message sent\n");
            ack = 1;
            return total_bytes_sent;  // Return total bytes sent so far
        }

        if (total_bytes_sent < 0)
            retries++;
    }

    if (ack == 0) {
        perror("No acknowledgment received");
        return total_bytes_sent;  // Return total bytes sent so far
    }

    return total_bytes_sent;  // Return total bytes sent
}


int rudp_recv(int sockfd, char *buffer, int len, struct sockaddr_in *src_addr){

    socklen_t addr_len = sizeof(*src_addr);

    // Receive data using recvfrom() function
    int bytes_received = recvfrom(sockfd, buffer, len, 0, (struct sockaddr *)src_addr, &addr_len);
    if (bytes_received < 0) {
        perror("recvfrom failed");
        return -1;
    }
    if(strcmp(buffer,"EXIT") == 0){//|| strcmp(buffer,"0") == 0){
        int i =rudp_close(sockfd);
        if (i<0)
        exit(EXIT_FAILURE);
        return -1;
    }
    // Acknowledge receipt of the packet
    char ack_packet[] = "ACK";
    sendto(sockfd, ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)src_addr, addr_len);

    return bytes_received;
}

int rudp_close(int sockfd) {

        if (sockfd < 0) {
        perror("Invalid socket descriptor\n");
        return -1;
    }

    // Close the socket
    if (close(sockfd) < 0) {
        perror("Error closing socket\n");
        exit(EXIT_FAILURE);
    }
    printf("socket closed\n");
    return 0;
}