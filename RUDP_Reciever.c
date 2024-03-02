
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

#define FILE_SIZE 2*1048576
#define BUFFER_SIZE 32741


void printTimes(double *time, int len, double sum) {

    printf("----------------------------------\n");
    printf("- * Statistics * -\n");
    for (int j = 0; j < len; j++) {
        printf("- RUN #%d Data : Time %.6f\n",j+1,*(time + j));
    }
    double avg = sum / len;
    double bandwidth = len / (avg * 1024 * 1024);

    printf("-\n");
    printf("- Average time:\t%0.6f\n", avg);
    printf("- Average bandwidth : %.6f \n",bandwidth);

    printf("----------------------------------\n");
    free(time);

}

int main(int argc, char *argv[]){

   // on linux to prevent crash on closing socket.
    signal(SIGPIPE, SIG_IGN);

    int port=-1;

    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: %s -p <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse port argument
    port = atoi(argv[2]);

    if (port == -1) {
        fprintf(stderr, "port must be provided.\n");
        fprintf(stderr, "Usage: %s -p <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Port: %d ", port);
    printf("Starting Receiver...\n");
    printf("Waiting for RUDP connection...\n");

    int size2=0;
    // on linux to prevent crash on closing socket.
    signal(SIGPIPE, SIG_IGN);
    char buffer[FILE_SIZE];
    char User_Decision_MESSAGE[1];

    char Check_DECISION[1];// create the message exit that the receiver will send if the user want to exit.
    Check_DECISION[0]='0';

    //Accept and incoming connection
    //printf("Waiting for incoming connections\n");

    struct sockaddr_in client_addr;
    socklen_t client_addr_length = sizeof(client_addr);

    double sum_for_average= 0.0;

    // allocating memory
    double *arrTime = NULL;
    arrTime = (double *) malloc(sizeof(double));
  //  arrTime = (double *) malloc(sizeof(double)*FILE_SIZE);
    if (arrTime == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    //arrTime= (double *) malloc(1);

    User_Decision_MESSAGE[0];
    int recevier_socket = -1;
    int countbytes = 0;

    int c = 0;
    int Bole=1;
    int rcv = 0;
    int t=1;

    recevier_socket = rudp_socket(2,port,0); // create the socket
    if (recevier_socket == -1) {
        exit(EXIT_FAILURE);
    }

    do{
        //printf("Sender connected, beginning to receive file...");
        countbytes=0;
        memset(&client_addr, 0, sizeof(client_addr)); 
        client_addr_length = sizeof(client_addr);

    while (1){
        struct timeval start, end;
        gettimeofday(&start, 0); 
        rcv = rudp_recv(recevier_socket, buffer, sizeof(buffer),&client_addr);
        if (rcv<0){
            Bole = 0;
            break;
        }

        //if(strcmp(buffer,"SYN")){
        // struct sockaddr_in dest_addr;
        // memset(&dest_addr, 0, sizeof(dest_addr));
        // dest_addr.sin_family = AF_INET;
        // dest_addr.sin_port = htons(port);
        // inet_pton(AF_INET, ip, &dest_addr.sin_addr);
        // if(strcmp(buffer,"SYN") == 0)
        // sendto(recevier_socket, "ACK", 3, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

        // if(strcmp(buffer,"EXIT") == 0){//|| strcmp(buffer,"0") == 0){
        // int i =rudp_close(recevier_socket);
        // if(i < 0)
        // exit(EXIT_FAILURE);
        // //free(arrTime);
        // //exit(EXIT_SUCCESS);
        // Bole = 0;
        // break;}//return;

            //send(recevier_socket, "ACK", 3,0);
         //   break;
      //  }
        
        countbytes += rcv;
        //printf("Received : %d bytes \n",countbytes);

        //printf("countbytes is %d compared to FILE_SIZE 2097152 \n",countbytes);
        if(countbytes >= FILE_SIZE){
                gettimeofday(&end, 0); // end measure

                printf("File transfer completed.\n");
                printf("Waiting for Sender response...\n");
                double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1e6;
                
                //printf("sum_for_average is %f\n",sum_for_average);
                sum_for_average += time_spent;
                //printf("sum_for_average is %f after sum_for_average += time_spent; \n",sum_for_average);

                //*(arrTime + c - 1) = time_spent;
                *(arrTime + c) = time_spent;
                //printf("c is %d \n",c);
                c++;
                //printf("c is %d after c++ \n",c);

                printf("Time in microseconds: %f microseconds\n", time_spent);
                
                size2+=countbytes;
                countbytes=0;


               // arrTime = (double *) realloc(arrTime, sizeof(double) * (c + 1));

                // To get the decision of the user
                bzero(User_Decision_MESSAGE, sizeof(User_Decision_MESSAGE));// update
                printf("\n\n");
                // get the message option from the user(sender)
                struct sockaddr_in filler;
                rudp_recv(recevier_socket, User_Decision_MESSAGE, sizeof(User_Decision_MESSAGE),&filler);
                printf("ACK sent.\n");
                if (User_Decision_MESSAGE[0] == Check_DECISION[0]) { 
                // if it's equal to exit-message then stop
                    Bole =0;
                    break;
                }
                    else{ //else continue
                        Bole =1;
                        size2=0;
                        printf("Receive %d\n",++t);
                    }

            }

        }
    }while(Bole);

    //printf("arrTime is %f \n",*arrTime+10);
    //printf("c is %d \n",c);
    //printf("sum_for_average is %f \n",sum_for_average);
    printTimes(arrTime,c,sum_for_average);
    //free(arrTime);
    close(recevier_socket);
    printf("Receiver end.\n");
    return 0;
}