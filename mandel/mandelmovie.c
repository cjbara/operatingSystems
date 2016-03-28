#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#define _GNU_SOURCE

int main(int argc, char *argv[]) {

	int numProcesses;

	//Get the number of processes to use or return error
	if(argc != 2) { 
		printf("usage: mandelmovie <numberOfProcesses>\n");
		return 1;
//	} else if (argc == 1) {
//		numProcesses = 1;
	} else {
		numProcesses = atoi(argv[1]);
	}

	//Create an integer to keep track of running processes
	int runningProcesses = 0;
	int moviesCompleted = 1;
	
	//Create the body to run the mandel command
	char *commands[100];
	commands[0] = "./mandel";
	commands[1] = " ";
	commands[2] = "-x";
	commands[3] = "0.286932";
	commands[4] = "-y";
	commands[5] = "0.014287";
	commands[6] = "-m";
	commands[7] = "2000";
	commands[8] = "-H";
	commands[9] = "600";
	commands[10] = "-W";
	commands[11] = "800";
	commands[12] = "-s";
	commands[13] = ".000001";
	commands[14] = "-o";
	commands[15] = "mandel50.bmp";
	commands[16] = "-n";
	commands[17] = "1";
	commands[18] = (char *) NULL;
	
	double targetZoom = .000001;
	double initialZoom = 2;
	double zoom = initialZoom;
	double delatZoom = exp(log(zoom/targetZoom)/50);

	//Enter the loop to do the work
	while (moviesCompleted <= 50) {
		
		//fork a new process
		if(runningProcesses >= numProcesses){
			//wait
			//printf("Waiting\n");
			int signal;
			int pid = wait(&signal);
			if(pid < 0){
			//	printf("No processes left to wait for, this should not be running\n");
			} else {
				if(signal == 0) {
					//wait worked normally
					runningProcesses--;
				} else {
					printf("Wait exited abnormally with signal %d: %s\n\n", signal, strsignal(signal));
				}
			}
		} else if (runningProcesses < numProcesses){
			//fork
			printf("Forking\n");
			//printf("r: %d n: %d m: %d\n", runningProcesses, numProcesses, moviesCompleted);
			int test;
			int pid = fork();
			if(pid == 0) {
				//child process
				//Calculate output file
				char *outfile = malloc(30);
				sprintf(outfile, "mandel%d.bmp", moviesCompleted);
				commands[15] = outfile;

				//calculate zoom commands[13]
				char *newZoom = malloc(20);
				int i;
				for(i = 1; i<moviesCompleted; i++) {
					zoom /= delatZoom;
				}
				sprintf(newZoom,"%lf", zoom);
				commands[13] = newZoom;
				printf("Starting %s\n", commands[15]);
				test = execvp(commands[0], &commands[1]);
				if (test < 0){
					printf("Could not start %s: %s\n", commands[0], strerror(errno));
					exit(1);
				}
			} else if(pid > 0) {
				//master process
				runningProcesses++;
				moviesCompleted++;
			} else {
				//parent process on failure
				printf("Could not fork master process: %s\n\n", strerror(errno));
				exit(1);
			}
		}
	}

	return 0;
	
}
