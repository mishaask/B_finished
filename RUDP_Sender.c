
#include <sys/time.h>
#include <errno.h>
#include <strings.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include "RUDP_API.h"

#define FILE_SIZE 2*1048576 //2MB

void generateRandomFile(const char *filename ,int size);


int main(int argc, char *argv[]){

    char *ip = NULL;
    int port = -1;

    if(argc != 5 || strcmp(argv[1], "-ip") != 0 || strcmp(argv[3], "-p") != 0){
        fprintf(stderr, "Usage: %s -ip <IP> -p <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ip = argv[2];
    port = atoi(argv[4]);

    if (ip == NULL || port == -1) {
        printf("All options (-ip, -p) are required.\n");
        exit(EXIT_FAILURE);
    }

    printf("Port: %d\n", port);
    printf("IP: %s\n", ip);

    struct sockaddr_in reciever_addr;
    memset(&reciever_addr, 0, sizeof(reciever_addr));
    reciever_addr.sin_family = AF_INET;
    reciever_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &reciever_addr.sin_addr);

    int User_Decision = 1;
    FILE *file = NULL;
    int count =0,sizeIn_byets=0,oneMbCounter=0;
    char User_Decision_MSG[1]; //  Save the user decision about to send again
    const char *filename = "random_file.txt";
    int file_size = FILE_SIZE; // Specify the size of the file in bytes
    generateRandomFile(filename, file_size);

   // int sender_socket = rudp_socket(1,dest_addr.sin_port ,dest_addr.sin_addr.s_addr);
    int sender_socket = rudp_socket(1,port,ip);
    if (sender_socket < 0) {
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    int j=0;

    while(1){// loop to countine until the user want to stop
        
        file = fopen(filename,"r");

        if(file == NULL){
            perror("Error opening file \n");
            exit(EXIT_FAILURE);
        }

        count++;
        printf("RUN %d\n",count);
        sizeIn_byets = rudp_send(sender_socket,file, port ,ip);
        if(sizeIn_byets<0){
            perror("Error while rudp_send file \n");
            sendto(sender_socket, "EXIT", sizeof("EXIT"), 0, (struct sockaddr *)&reciever_addr, sizeof(reciever_addr));
            exit(EXIT_FAILURE);
        }

        oneMbCounter+=sizeIn_byets;
        printf("%d\n",oneMbCounter);


        if(oneMbCounter >= FILE_SIZE){
            printf("sent all the %d MB file (%d byets) \n", FILE_SIZE/1048576,oneMbCounter);
            fclose(file);

            printf("User Decision:-\nPress 1 to continue ,0 to exit!\n");
            scanf("%d", &User_Decision);
            printf("*******************************\n");
            
            if(User_Decision == 0){
                User_Decision_MSG[0]='0';
            
            }else{
                User_Decision_MSG[0]='1';
            }

            printf("\nuser chose %d\n",User_Decision);
        //send the decision.
        //     struct sockaddr_in dest_addr;
        // memset(&dest_addr, 0, sizeof(dest_addr));
        // dest_addr.sin_family = AF_INET;
        // dest_addr.sin_port = htons(port);
        // inet_pton(AF_INET, ip, &dest_addr.sin_addr);

        //send the decision.
        sendto(sender_socket, User_Decision_MSG, sizeof(User_Decision_MSG), 0, (struct sockaddr *)&reciever_addr, sizeof(reciever_addr));

        }else{
            printf("sent just %d out of %d\n", oneMbCounter, FILE_SIZE);
        }

        if (User_Decision==0){
            break;
        }
        j++;
        sizeIn_byets = 0;
        oneMbCounter = 0;
    }

    int i = rudp_close(sender_socket);
    if(i<0){
        perror("Error while closing socket \n");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}





//Function to generate a file with random content
void generateRandomFile(const char *filename, int size) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Seed the random number generator
    srand(time(NULL));

    // Generate random content and write it to the file
    for (int i = 0; i < size; ++i) {
        // Generate a random character (ASCII value between 0 and 255)
        char random_char = rand() % 256;
        // Write the character to the file
        fputc(random_char, file);
    }

    fclose(file);
}