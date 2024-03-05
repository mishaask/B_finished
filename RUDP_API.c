
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


// Define a struct for the packet header
typedef struct {
    int seq_num;
    int packet_size;
    uint16_t checksum;
    char data[BUFFER_SIZE];
} PacketHeader;

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
    memset(&reciever_address,0,sizeof(reciever_address));
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

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Create a UDP socket
    int sockfd = -1;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("Couldn't create a socket : %d\n",errno);
        perror("socket creation failed");
        return -1;
    }

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
    int total_bytes_sent = 0;

    struct sockaddr_in dest_addr;
    socklen_t length = sizeof(dest_addr);
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &dest_addr.sin_addr);

    while (ack == 0 && retries < TRANSMITION_TRIES) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
        if (bytes_read == 0) {
            if (feof(file)) {
                printf("End of file reached.\n");
            } else {
                perror("Error reading from file");
            }
            return total_bytes_sent;  // Return total bytes sent so far
        }

        // Create a packet header
        PacketHeader header;
        header.seq_num = retries;
        header.packet_size = bytes_read;

        // Calculate checksum
        header.checksum = calculate_checksum(buffer, bytes_read);
        
        // Copy data to header
        memcpy(header.data, buffer, bytes_read);
        ssize_t bytes_sent = sendto(client_socket, (char *)&header, sizeof(PacketHeader), 0, (struct sockaddr *)&dest_addr, length);
        if (bytes_sent < 0 && retries >= TRANSMISSION_TRIES) {
            perror("Sendto failed");
            exit(EXIT_FAILURE);
        }

        total_bytes_sent += bytes_sent;  // Update total bytes sent

        // Receive ACK from sender
        char ack_buffer[3] = {0};
        recvfrom(client_socket, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&dest_addr, &length);

        if (strcmp(ack_buffer, "ACK") == 0) {
            printf("Received ACK from sender. Sending ACK...\n");
            printf("Message sent\n");
            ack = 1;
            return total_bytes_sent; 
        }

        if (total_bytes_sent < 0)
            retries++;
    }

    if (ack == 0) {
        perror("No acknowledgment received");
        return total_bytes_sent; 
    }

    return total_bytes_sent; 
}


int rudp_recv(int sockfd, char *buffer, int len, struct sockaddr_in *src_addr){

    PacketHeader header;
    socklen_t addr_len = sizeof(*src_addr);

    // Receive data
    int bytes_received = recvfrom(sockfd, (char *)&header, sizeof(PacketHeader), 0, (struct sockaddr *)src_addr, &addr_len);
    if (bytes_received < 0) {
        perror("recvfrom failed");
        return -1;
    }

    if(strcmp(buffer,"EXIT") == 0){
        int i =rudp_close(sockfd);
        if (i<0)
        exit(EXIT_FAILURE);
        return -1;
    }

    // Verify checksum
    uint16_t checksum = calculate_checksum(header.data, header.packet_size);
    if (checksum != header.checksum) {
        printf("Checksum verification failed. Packet ignored.\n");
        return -1;
    }


    // Copy data from header to buffer
    memcpy(buffer, header.data, header.packet_size);

    // Acknowledge receipt of the packet
    char ack_packet[] = "ACK";
    sendto(sockfd, ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)src_addr, addr_len);

    return header.packet_size;
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

// Function to calculate checksum
uint16_t calculate_checksum(char *data, size_t size) {
    uint16_t checksum = 0;
    for (int i = 0; i < size; i++) {
        checksum += data[i];
    }
    return checksum;
}