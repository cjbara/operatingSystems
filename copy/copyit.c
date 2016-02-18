/* Program that has the functionality of cp in the unix shell
   By: Cory Jbara
   Date: February 2, 2016 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

int checkArgs(int argc);
void delayAlert(int s);

int main(int argc, char **argv) {

	//Check if the function was called properly
	checkArgs(argc);

	//Set up the signal
	signal(SIGALRM,delayAlert);
	alarm(1);

	//Open the file from argv[1]
	char *infilename = argv[1];
	int infile = open(infilename, O_RDONLY, 0);
	if(infile < 0){
		printf("Unable to open %s: %s\n",infilename,strerror(errno));
		exit(1);
	}

	//Create the output file from argv[2]
	char *outfilename = argv[2];
	int outfile = creat(outfilename, 00644);
	if(outfile < 0){
		printf("Unable to open %s: %s\n",outfilename,strerror(errno));
		exit(1);
	}

	//Initialize buffers and result 
	int readSize = 4096;
	void *buffer = malloc(readSize);
	int result = readSize;
	int totalBytes = 0;

	//Read file until there are less than readSize bytes
	while(result == readSize){

		//Read the file into buffer
		result = read(infile, buffer, readSize);
		while(result < 0){
			if(errno == EINTR){
				result = read(infile, buffer, readSize);
			} else {
				printf("Unable to read from %s: %s\n",infilename,strerror(errno));
				exit(1);
			}
		}
		
		//write the file from buffer	
		result = write(outfile, buffer, result);
		while(result < 0){
			if(errno == EINTR){
				result = write(outfile, buffer, result);
			} else {
				printf("Unable to write to %s: %s\n",infilename,strerror(errno));
				exit(1);
			}
		}

		//Add up the number of bytes 
		totalBytes += result;
	}
	
	//Close the two files
	close(infile);
	close(outfile);

	//Print success message and return
	printf("copyit: Copied %d bytes from %s to %s\n", totalBytes, infilename, outfilename);
	return 0;
}

//Checks if we have the proper number of arguments
int checkArgs(int argc) {
	if(argc > 3){
		printf("copyit: Too many arguments\n");
		printf("usage: copyit <sourcefile> <targetfile>\n");
		exit(1);
	} else if(argc < 3) {
		printf("copyit: Not enough arguments\n");
		printf("usage: copyit <sourcefile> <targetfile>\n");
		exit(1);
	} else {
		return 0;
	}
}

//Displays a delay alert
void delayAlert(int s){
	printf("copyit: Still copying...\n");
	alarm(1);
}
