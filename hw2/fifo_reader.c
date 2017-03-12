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

int main(int argc, char const *argv[])
{
	int i;
	int j;
	int checkIfBreak = 0;
	int fd; // file discriptor
	char written[4096];
	int numOfa;
	struct timeval t1,t2;
	double elapsed_milisec;

	struct sigaction new_action1,old_action1;//for SIGINT
	new_action1.sa_handler = SIG_IGN;
	new_action1.sa_flags = 0;
	if(sigaction(SIGINT, &new_action1, &old_action1) !=0 )
		sendError("Signal handle registration failed")
	
	sleep(2); // to be sure that the fifo created


	fd = open(FIFOPATH,O_RDONLY);
	if ( fd == -1)
		sendError("Error opening fifo pipe")

	i =  gettimeofday(&t1, NULL);//start timing
	if (i == -1)
		sendError("Error time checking")

	i = read(fd,written,4096);
	if(i == -1)
		sendError("Error reading from file")

	while(i != -1 && i !=0){
		for (j = 0; j < i; j++) {
			if(written[j] == 'a')
				numOfa = numOfa + 1;
		}

		i = read(fd,written,4096);
		if(i == -1)
			sendError("Error reading from file")
	}

	i = gettimeofday(&t2,NULL);//end timing
	if ( i == -1)
		sendError("Error time checking")

	elapsed_milisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsed_milisec += (t2.tv_usec - t1.tv_usec) / 1000.0;

	printf("%d were read in %f microseconds through FIFO\n",numOfa,elapsed_milisec);

	i = close(fd);
	if( i == -1)
		sendError("Error closing fifo pipe")

	if(sigaction(SIGINT, &old_action1, NULL) !=0 )//for SIGITN
		sendError("Signal handle recover failed")

	exit(EXIT_SUCCESS);
}