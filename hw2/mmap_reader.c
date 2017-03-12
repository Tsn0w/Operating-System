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

int signal_info = 0;
void signal_handler1(int signum);

int main(int argc, char const *argv[])
{
	struct sigaction new_action1,old_action1;
	new_action1.sa_handler = SIG_IGN;
	new_action1.sa_flags = 0;
	if(sigaction(SIGTERM, &new_action1, &old_action1) !=0 )
		sendError("Signal handle registration failed")

	int FileSize;
	struct timeval t1,t2;
	struct sigaction new_action,old_action;
	char *arrmap;
	new_action.sa_handler = signal_handler1;
	new_action.sa_flags = 0;
	int numOfa = 0;
	int fd; //file discriptor
	int i; // for check errors
	int j;
	double elapsed_milisec;
	
	if(sigaction(SIGUSR1, &new_action, &old_action) !=0 ){
		sendError("Signal handle registration failed")
	}
	while(!signal_info){
		sleep(2);
	}

	fd = open(FILEPATH, O_RDONLY , 0600);
	if (fd  == -1)
		sendError("Error opening file")

	FileSize = lseek(fd,0,SEEK_END);//check the file size
	if (FileSize == -1){
		sendError("Error lseeking to the end")
	}

	i =  gettimeofday(&t1, NULL);//start timing
	if (i == -1)
		sendError("Error time checking")
	

	arrmap = (char*) mmap(NULL,FileSize,PROT_READ | PROT_WRITE, MAP_SHARED,fd,0);//make mmap
	if (MAP_FAILED == arrmap){
		sendError("Error creating mmap")
	}
	for (j = 0; j <FileSize - 1; j++){
		if( arrmap[j] != 'a')
			break;
		numOfa++;
	}
	if (arrmap[j] != '\0' || j != FileSize - 1){
		printf("last char read is not 'a' or last char is not null terminator");
		exit(-1);
	}

	i = gettimeofday(&t2,NULL);//end timing
	if ( i == -1)
		sendError("Error time checking")

	elapsed_milisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsed_milisec += (t2.tv_usec - t1.tv_usec) / 1000.0;

	printf("%d were written in %f microseconds through MMAP\n",numOfa+1,elapsed_milisec);
		
	if ( sigaction(SIGUSR1, &old_action, NULL) != 0)
	sendError("Error sending old action")

	i = close(fd);
	if (i == -1)
		sendError("Error closing file")

	i = unlink(FILEPATH);
	if(i == -1)
		sendError("Error unlinking,removing Disk from file")

	munmap(arrmap,FileSize);
	if (MAP_FAILED == arrmap)
  		sendError("Error closing mmap")

  	if(sigaction(SIGTERM, &old_action1, NULL) !=0 )
		sendError("Signal handle registration failed")
	
	exit(EXIT_SUCCESS);
}

// Signal handler.
// Simulate some processing and finish
void signal_handler1 (int signum)
{
	signal_info = 1;
}
