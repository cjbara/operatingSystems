#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#define _GNU_SOURCE

int main() {
	char *prompt = "myshell> ";
	int numCommands = 0;
	char *commands[101];
	char *buffer = malloc(4096);
	
	while(1){
		//Reset the number of commands and print the prompt
		numCommands = 0;
		printf("%s", prompt);
		fflush(stdout);

		//Get input command from the user and store it into buffer
		if(!fgets(buffer, 4096, stdin)) {
			printf("myshell: end of input\n\n");
			exit(1);
		}

		//Pull words from the command and store them into buffer
		char *c;
		c = strtok(buffer, " \t\n");
		if( c == NULL ){
			continue;
		}

		//take words from buffer and store them into the commands array
		while(c) {
			commands[numCommands++] = c;
			c = strtok(0, " \t\n");
			if(numCommands >= 100){
				printf("myshell: cannot accept commands of more than 100 words\n\n");
				break;
			}
		}
		
		//Do not accept commands with more than 100 words
		if(numCommands >= 100) { continue; }

		commands[numCommands] = (char *) NULL;

		//Test the first word against known commands to determine proper myshell command
		if(strcmp(commands[0], "start") == 0) {
			//code for start
			//Check if there are no commands being called
			if(numCommands == 1){
				printf("myshell: enter a UNIX command after the myshell start command\n\n");
				continue;
			}
			int test;
			int pid = fork();
			if(pid == 0) { 
				//child process, execute new process
				test = execvp(commands[1], &commands[1]);
				if(test < 0) {
					printf("myshell: could not start process: %s\n\n", strerror(errno));
				}
			} else if(pid > 0) {
				//parent process, print success message
				printf("myshell: process %d started\n\n", pid);
			} else {
				//negative, parent process on failure
				printf("myshell: could not start process: %s\n\n", strerror(errno));
			}
					
		} else if(strcmp(commands[0], "wait") == 0) {
			//code for wait
			//Check if there is another commands being called
			if(numCommands > 1){
				printf("myshell: myshell wait command does not take any arguments\n\n");
				continue;
			}
			int signal;
			int pid = wait(&signal);
			if(pid < 0) {
				printf("myshell: no processes left\n\n");
			} else {
				if(signal == 0){
					printf("myshell: process %d exited normally with signal 0\n\n", pid);
				} else {
					printf("myshell: process %d exited abnormally with signal %d: %s\n\n", pid, signal, strsignal(signal));
				}
			}
			
		} else if(strcmp(commands[0], "run") == 0) {
			//code for run
			//Check if there are no commands being called
			if(numCommands == 1){
				printf("myshell: enter a UNIX command after the myshell run command\n\n");
				continue;
			}
			int test;
			int pid = fork();
			if(pid == 0) { 
				//child process, execute new process
				test = execvp(commands[1], &commands[1]);
				if(test < 0) {
					printf("myshell: could not start process: %s\n\n", strerror(errno));
				}
			} else if(pid > 0) {
				//parent process, wait for process to finish
				int signal;
				pid = waitpid(pid, &signal, 0);
				if(pid < 0){
					printf("myshell: error: %s\n\n", strerror(errno));
				} else {
					if(signal == 0){
						printf("myshell: process %d exited normally with signal 0\n\n", pid);
					} else {
						printf("myshell: process %d exited abnormally with signal %d: %s\n\n", pid, signal, strsignal(signal));
					}
				}
			} else {
				//negative, parent process on failure
				printf("myshell: could not start process: %s\n\n", strerror(errno));
			}

		} else if(strcmp(commands[0], "kill") == 0) {
			//code for kill
			//Check if there are no commands being called
			if(numCommands == 1){
				printf("myshell: enter a process ID (pid) after myshell kill command\n\n");
				continue;
			}
			int result = kill(atoi(commands[1]), SIGKILL);
			if(result == 0) {
				printf("myshell: process %d killed\n\n", atoi(commands[1]));
			} else if(result < 0) {
				//display error message
				printf("myshell: could not kill process: %s\n\n", strerror(errno));
			}

		} else if(strcmp(commands[0], "stop") == 0) {
			//code for stop
			//Check if there are no commands being called
			if(numCommands == 1){
				printf("myshell: enter a process ID (pid) after myshell stop command\n\n");
				continue;
			}
			int result = kill(atoi(commands[1]), SIGSTOP);
			if(result == 0) {
				printf("myshell: process %d stopped\n\n", atoi(commands[1]));
			} else if(result < 0) {
				//display error message
				printf("myshell: could not stop process: %s\n\n", strerror(errno));
			}

		} else if(strcmp(commands[0], "continue") == 0) {
			//code for continue
			//Check if there are no commands being called
			if(numCommands == 1){
				printf("myshell: enter a process ID (pid) after myshell continue command\n\n");
				continue;
			}
			int result = kill(atoi(commands[1]), SIGCONT);
			if(result == 0) {
				printf("myshell: process %d continued\n\n", atoi(commands[1]));
			} else if(result < 0) {
				//display error message
				printf("myshell: could not continue process: %s\n\n", strerror(errno));
			}

		} else if((strcmp(commands[0], "quit") == 0) || (strcmp(commands[0], "exit") == 0)) {
			exit(0);

		} else {
			//code for other command
			printf("myshell: unknown command: %s\n\n", commands[0]);
		}
	}

	free(buffer);
	return 0;
}
