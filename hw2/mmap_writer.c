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

#define FILEPATH "/tmp/mmapped.bin"
#define sendError(error) {printf("%s %s: %d\n",(error), strerror(errno), __LINE__); return errno;}

int main(int argc, char const *argv[]){
	if(argc != 3)
		sendError("Wrong number of inputs in command line")

	struct sigaction new_action1,old_action1;
	new_action1.sa_handler = SIG_IGN;
	new_action1.sa_flags = 0;
	if(sigaction(SIGTERM, &new_action1, &old_action1) !=0 )
		sendError("Signal handle registration failed")
	

	unsigned long NUM =  strtol(argv[1], NULL, 10);
	int RPID = (int) strtol(argv[2], NULL, 10);

	int i = 0; //for check errors
	int fd; // file descirptor
	char *arrmap;
	struct timeval t1,t2;
	double elapsed_milisec;
	int written = 0;

	fd = open(FILEPATH, O_WRONLY | O_CREAT,0600);//open file
	if (fd == -1)
		sendError("Error opening file")


	i = fchmod(fd,0600);//give premission
	if ( i == -1 )
		sendError("Error set premissions")


	i = lseek(fd, NUM-1, SEEK_SET);
  	if (-1 == i)
		sendError("Error calling lseek() to 'stretch' the file")

	  i = write(fd, "", 1);
	  if (1 != i)
    	sendError("Error writing last byte of the file")

  	arrmap = (char*) mmap(NULL,NUM,PROT_READ | PROT_WRITE, MAP_SHARED,fd,0);//make mmap
  	if (MAP_FAILED == arrmap)
  		sendError("Error creating mmap")

  	//start time counting
  	i =  gettimeofday(&t1, NULL);
  	if ( i == -1)
  		sendError("Error time checking")

  	for (written = 0; written < NUM; ++written){ //write NUM-1 'a'
    	arrmap[written] = 'a';
    }
    arrmap[NUM-1] = '\0';

    kill(RPID, SIGUSR1); // send signal

    i = gettimeofday(&t2, NULL);
    if( i == -1)
    	sendError("Error time checking")


    elapsed_milisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsed_milisec += (t2.tv_usec - t1.tv_usec) / 1000.0;

	printf("%d were written in %f microseconds through MMAP\n",written,elapsed_milisec);
	
	i = munmap(arrmap,NUM);
	if (MAP_FAILED == arrmap)
  		sendError("Error closing mmap")

	i = close(fd);
	if (i == -1)
		sendError("Error closing file")

	exit(EXIT_SUCCESS);
}