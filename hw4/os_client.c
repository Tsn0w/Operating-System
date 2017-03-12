#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> 
#include <fcntl.h>


#define sendError(error){\
    printf("%s %s: %d\n",(error), strerror(errno), __LINE__);\
    exit(errno);\
}
#define BUFFSIZE 2048

int main(int argc, char const *argv[])
{
    if (argc != 5)
        sendError("Error, wrong numbe of arguments")

    char const *IP = argv[1]; 
    short PORT = strtol (argv[2], NULL, 10);
    char const *IN = argv[3];
    char const *OUT = argv[4];

    struct sockaddr_in serv_addr = {0};
    socklen_t addrsize = sizeof(struct sockaddr_in);
    int sockfd;

    char *data_To_Send[2048] = {0} ;
    char *data_Received[2048] = {0};

    int IN_File_Dis;
    int OUT_File_Dis;
    
    int num_Sent = 0;
    int sent = 0;
    int num_Rec = 0;
    int num_Read_From_File = 0;
    int total_Rec = 0;
    int wrote = 0;
    int total_wrote = 0;

    
    //create socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        sendError("Error, could not create client socket")


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(IP);


    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        sendError("Error, Failed connect to server")


    //open OUT file
    if( (OUT_File_Dis = open(OUT,O_WRONLY | O_TRUNC | O_CREAT, 0777)) < 0 )
        sendError("Error, opening OUT file")
    
    
    //open IN file 
    if ((IN_File_Dis = open(IN,O_RDONLY)) < 0 )
        sendError("Error opening IN file")
    
    
    //first read from file
    if( (num_Read_From_File = read(IN_File_Dis,data_To_Send, BUFFSIZE)) < 0 )
        sendError("error reading file")
    
    //check if the file is empty.
    if (num_Read_From_File == 0)
        sendError("Error file is empty")


    int sum = 0;

    while( num_Read_From_File > 0 ){

        //to ensure we send all we read.
        while (num_Sent < num_Read_From_File ){
            
            
            if ( (sent = send(sockfd, data_To_Send + num_Sent,num_Read_From_File - num_Sent , 0)) < 0 ) 
                sendError("Error, send bytes to server")

            num_Sent += sent;
        }


        //to ensure we receive all we send
        while ( total_Rec < num_Sent){
            if( (num_Rec = recv(sockfd, data_Received + total_Rec, num_Sent - total_Rec,0)) < 0 )
                sendError("Error, receeiving bytes form server")
            
            total_Rec += num_Rec;
        }


        //to ensure we wrote all we received
        while (total_wrote < total_Rec){

            if( (wrote = write(OUT_File_Dis, data_Received + total_wrote, num_Sent - total_wrote)) < 0 )
                sendError("Error, unable to write to OUT file")

            total_wrote += wrote;
        }


        if( (num_Read_From_File = read(IN_File_Dis,data_To_Send, BUFFSIZE)) < 0 )
            sendError("error reading file")
        
        
        num_Sent = 0;
        total_Rec = 0;
        total_wrote = 0;
    }

    //close IN & OUT files
    if( close(IN_File_Dis) < 0 )
        sendError("Error, closing IN file") 

    if( close(OUT_File_Dis) < 0 )
        sendError("Error, closing OUT file")

    //clsoe socket
    if ( close(sockfd) < 0 )
        sendError("Error, closing sokcet")

    return 0;
}