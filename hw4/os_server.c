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
	if(errno != EINTR){\
    	printf("%s %s: %d\n",(error), strerror(errno), __LINE__);\
    	exit(errno);\
	}\
}
#define RandFile "/dev/urandom"
#define BUFFSIZE 2048

void handler(int signum);

/*---------- Global Variable -------------------*/
	char randoms[2048] = {0}; // to save random from /dev/urand
 	char Received_arr[2048] = {0}; // to save Received data from 
 	int connfd;
 	int keyfd;
 	int sockfd;

/*----------- Main ----------------------------*/
int main(int argc, char const *argv[])
{
	if ( argc != 4 && argc != 3)
		sendError("Error, wrong arguments in commend line")
	
    struct sockaddr_in serv_addr = {0};

	int PORT = strtol(argv[1],NULL,10);
	char const *KEY = argv[2];

	
	
	int random_Data_Len = 0;
	int Rand_Read;
	int randfd;
	int KEYLEN;

	// need to check if key has data nad exist
	//if so dont ahve data or not exist exit.
	
	//in case KEYLEN is not given
	if (argc == 3){
		struct stat key_Stat;
		
		if (stat(KEY,&key_Stat) != 0)
			sendError("Error, KEY file does not exist")

		if( (KEYLEN = key_Stat.st_size) == 0)
			sendError("Error, KEY file is empty")
	}

	//in case KEYLEN is given
	if(argc == 4){
	
		//open file to write
		if( (keyfd = open(KEY, O_WRONLY | O_TRUNC | O_CREAT, 0700)) < 0)
			sendError("Error, cant open/create the KEY file")
		
		KEYLEN = strtol(argv[3],NULL,10); 

		//open /dev/urandom to get randoms
		if( (randfd = open(RandFile, O_RDONLY)) < 0 )
			sendError("Error, open /dev/urandom")

		//to be ensure we filled KEY file with KEYLEN randoms
		while (random_Data_Len < KEYLEN){

			if ( (Rand_Read = read(randfd, randoms + random_Data_Len, KEYLEN - random_Data_Len)) < 0 )
		        sendError("Error, unable to read from /dev/urandom")
		    random_Data_Len += Rand_Read;
		}

		if( close(randfd) != 0 )
			sendError("Error, can not close /dev/urandom")

		int written = 0;
		int wrote;

		//to be ensure we wrote KEYLEN randoms to KEY file 
		while (written < KEYLEN){
			
			if( (wrote = write(keyfd,randoms + written, KEYLEN - written)) < 0 )
				sendError("Error, unable to write to KEY file ")

			written += wrote;
		}

		if( close(keyfd) != 0 )
			sendError("Error, can not close KEY file")

	}

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		sendError("Error, can not create socket")
    

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any local machine address
    serv_addr.sin_port = htons(PORT); 

    //bind server 
    if(bind(sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0 )
    	sendError("Error, bind Failed")

    if(listen(sockfd, 10) < 0)
    	sendError("Error, listen Failed")
    


    //craete signal handler for SIGINT
    struct sigaction new_action,old_action;//for SIGINT
	new_action.sa_handler = handler;//TODO, SIG_IGN -> handler func name
	new_action.sa_flags = 0;
	if(sigaction(SIGINT, &new_action, &old_action) !=0 )
		sendError("Signal handle registration failed")
 
 	pid_t pid;
 	int total_rd = 0;
 	int rd;
 	int i;
 	int num_Sent = 0;
 	int sent = 0;
 	int Rec = 0;


 	while(1){

   		//new connection 
        if( (connfd = accept(sockfd, NULL, NULL)) < 0 )
        	sendError("Error, accept Failed")
		
		//create new child
 		if( (pid = fork()) < 0 )
   			sendError("Error, unable to fork")

   		if( pid == 0 ){

   			//open KEY file
   			if( (keyfd = open(KEY,O_RDONLY)) < 0 )
   				sendError("Error, unable open KEY file")

   			while ((Rec = recv(connfd, Received_arr , BUFFSIZE, 0)) > 0){

	   			// to ensure we read as much we received
	   			while(total_rd < Rec){
	   				if( (rd = read(keyfd, &randoms[total_rd], Rec - total_rd)) < 0 )
	   					sendError("Error, unable to read KEY file")
	   				total_rd += rd;

	   				if (rd == 0)
	   					if(lseek(keyfd, 0,SEEK_SET) < 0 )
	   						sendError("Error, unable lseeking")
	   			}
	   			
	   			//encrypting
	   			for(i = 0; i < Rec; i++)
	   				Received_arr[i] = Received_arr[i] ^ randoms[i];

	   			while (num_Sent < Rec){

	   				if ( (sent = send(connfd, &Received_arr[num_Sent],Rec - num_Sent , 0)) < 0 ) 
                		sendError("Error, send bytes to server")
          			  num_Sent += sent;
	   			}

   				total_rd = 0;
   				num_Sent = 0;
	   		}
			
			if (Rec < 0)
				sendError("Error, unable to receive data from client")

	   		//close connection
	   		if(close(connfd) < 0 )
	   			sendError("Error, unable to close connection")

	   		//close socket
	   		if(close(sockfd) < 0 )
	   			sendError("Error, unable to close connection")
	   		
	   		//kill child process
	   		if(kill(getpid(), SIGKILL) < 0 )
   				sendError("Error , unable to kill child process")
	   		

   		}

 	}

 	// // restore SIGINT old pointer
 	// if ( sigaction(SIGINT, &old_action, NULL) != 0)
		// sendError("Error sending old action")

	return 0;
}

void handler(int signum){

	//to check of connected is closed	
	if (fcntl(connfd, F_GETFD) != -1 || errno != EBADF)
		close(connfd);

	//to check if socket is closed
	if (fcntl(sockfd, F_GETFD) != -1 || errno != EBADF)
		close(sockfd);

	//to check if KEY file is closed
	if (fcntl(keyfd, F_GETFD) != -1 || errno != EBADF)
		close(keyfd);

	exit(0);
} 