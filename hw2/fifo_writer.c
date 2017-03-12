#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

#define sendError(error) {printf("%s %s: %d\n",(error), strerror(errno), __LINE__); return errno;}
#define FIFOPATH "/tmp/osfifo"


int main(int argc, char const *argv[]){
	
	if (argc != 2)
		sendError("Wrong nubmer of inputs in command line")

	struct sigaction new_action1,old_action1;//for SIGINT
	new_action1.sa_handler = SIG_IGN;
	new_action1.sa_flags = 0;
	if(sigaction(SIGINT, &new_action1, &old_action1) !=0 )
		sendError("Signal handle registration failed")

	struct sigaction new_action2,old_action2; //for SIGPIPE
	new_action2.sa_handler = SIG_IGN;
	new_action2.sa_flags = 0;
	if(sigaction(SIGINT, &new_action2, &old_action2) !=0 )
		sendError("Signal handle registration failed")

	int i; //for check errors
	int j; 
	int numToRead;
	int fd; // file disciptor
	double elapsed_milisec;
	long NUM =  strtol(argv[1], NULL, 10);
	struct timeval t1,t2;
	char ln = '\0';
	char listOfa[4096];
	struct stat isFifo;

	for (i = 0; i < 4096; ++i)
		listOfa[i] = 'a';

	i = mkfifo(FIFOPATH,0600); // create fifo
	if( i == -1){
		if(errno != EEXIST)	
			sendError("Error creating fifo pipe")
		if(stat(FIFOPATH,&isFifo) < 0)
			sendError("Error use stat function")
		if(S_ISFIFO(isFifo.st_mode) < 0 )
			sendError("Error, file is not fifo file")
		if(chmod(FIFOPATH,0600) < 0)
			sendError("Error, assigning permissions")
	}
	


	fd = open(FIFOPATH,O_WRONLY); // open fifo
	if (fd == -1)
		sendError("Error opening fifo file")

	i =  gettimeofday(&t1, NULL);//start timing
	if (i == -1)
		sendError("Error time checking")

	while ( NUM > 0){
		numToRead = NUM <= 4096 ? NUM : 4096;
		i = write(fd,listOfa,numToRead);
		j = j+numToRead;
		if ( i == -1){
			printf("Error writting in pipe");
	 		if (errno == EPIPE){
	 			i = gettimeofday(&t2,NULL);//end timing in case of no reader pipe
				if ( i == -1)
					sendError("Error time checking")

				elapsed_milisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
				elapsed_milisec += (t2.tv_usec - t1.tv_usec) / 1000.0;

				printf("%d were written in %f microseconds through FIFO\n",j,elapsed_milisec);

	 		}
		}
		NUM = NUM - 4096;
	}

	i = write(fd,&ln,sizeof(ln));
	if(i == -1){
		printf("Error writting \n");
		if (errno = EPIPE){
			elapsed_milisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
			elapsed_milisec += (t2.tv_usec - t1.tv_usec) / 1000.0;

			printf("%d were written in %f microseconds through FIFO\n",j,elapsed_milisec);

		}
		return -1;
	}

	i = gettimeofday(&t2,NULL);//end timing
	if ( i == -1)
		sendError("Error time checking")

	elapsed_milisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsed_milisec += (t2.tv_usec - t1.tv_usec) / 1000.0;

	printf("%d were written in %f microseconds through FIFO\n",j,elapsed_milisec);


	i = unlink(FIFOPATH);//unlinking fifo pipe
	if(i == -1)
		sendError("Error unlinking,removing Disk from file")

	i = close(fd); //close fifo pipe
	if ( i == -1)
		sendError("Error closing fifo pipe");
	if(sigaction(SIGINT, &old_action1, NULL) !=0 )//for SIGITN
		sendError("Signal handle recover failed")

	if(sigaction(SIGPIPE, &old_action2, NULL) !=0 )//for SIGPIPE
		sendError("Signal handle recover failed")

	exit(EXIT_SUCCESS);
}


