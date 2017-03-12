#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <unistd.h>
#include <time.h> 
#include <sys/time.h>
#include <assert.h>
#include <errno.h> 
#include <string.h>

#define sendError(error) {printf("%s %s\n",(error), strerror(errno)); return -1;}

int main(int argc, char **argv){

	DIR *dirin;
	DIR *dirout;
	char *dirInPath;
	char*dirOutPath;
	struct dirent *originalFile;
	dirInPath = argv[1];
	dirOutPath = argv[3];

	assert ( argc == 4 );

	//Open dir with files.
	if ((dirin = opendir(dirInPath)) == NULL)
		sendError("Error opening directory")

	//Open the output dir, and create if needed. 
	if ((dirout = opendir(dirOutPath)) == NULL){
		int newDir = mkdir(dirOutPath, 0777);
		if ( newDir < 0)
			sendError("Error opening directory")
	}

	char keyFilenName[1024];
	char fileName[1024];
	char outPutFile[1024];
	int createFileInfo;
	int fileDiscriptor;
	int keyDiscriptor;
	char readKey[1];
	char readFile[1];
	char XORedBit[1];
	int numReadFromFile;
	int numReadFromKey;
	int written;
	struct stat filestat;
	int test=1;



	//open the key file.
	keyDiscriptor = open(argv[2],O_RDONLY | O_TRUNC);
	if (keyDiscriptor < 0)
		sendError("Error opening Key file")

	while ((originalFile =readdir(dirin)) != NULL){

		sprintf(fileName, "%s/%s", dirInPath,originalFile->d_name);
		sprintf(outPutFile, "%s/%s", dirOutPath,originalFile->d_name);

		if(stat(fileName,&filestat)<0)
			sendError("Error getting stat of file")

		//check if file or directroy.
		if(!S_ISDIR(filestat.st_mode)){
			printf("in:%s)",originalFile->d_name);

			//create file.
			createFileInfo = creat(outPutFile, S_IRWXU|S_IRWXG|S_IRWXO);
			if (createFileInfo < 0 )
				sendError("Error creating file")

			//open the flie in the directroy.
			fileDiscriptor = open(fileName,O_RDONLY | O_TRUNC);
			if (fileDiscriptor < 0 )
				sendError("Error opening file")

			//open the file created.
			createFileInfo = open(outPutFile,O_WRONLY | O_TRUNC);
			if (createFileInfo < 0)
				sendError("Error opening file")

			numReadFromFile = read(fileDiscriptor,readFile, 1);
			//check if the file is empty.
			if ( test = 1 & numReadFromFile <= 0)
				sendError("Error reading file")


			while (numReadFromFile > 0){
		
					numReadFromKey = read(keyDiscriptor,readKey,1);
					//check if the key is empty, done only one time.
					if (test == 1 & numReadFromKey == 0)
						sendError("Error,Key is empty")
					//check if we  done reading the key.
					if (numReadFromKey == 0){
						close(keyDiscriptor);
						keyDiscriptor = open(argv[2],O_RDONLY | O_TRUNC);
						numReadFromKey = read(keyDiscriptor,readKey,1);
					}
					test=2;

					XORedBit[0] = readKey[0]^readFile[0];
					written = write(createFileInfo,XORedBit[0],1);

					numReadFromFile = read(fileDiscriptor,readFile,1);
					//check if the file is empty.
					if ( test = 1 & numReadFromFile < 0)
						sendError("Error reading file")

				}

				//end of encrypt/decrypt file.
				close(fileDiscriptor);
				close(createFileInfo);
				close(keyDiscriptor);
			}
					
		}		
	return 0;
}
