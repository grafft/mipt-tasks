#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>
#include <limits.h>

void error_prex(char* msg);

int main(int argc, char** argv, char** envp){
    if(argc < 2){
        printf("Invalid number of arguments. Usage: %s <client_count>\n", argv[0]);
        exit(-1);
    }
    int m = strtol(argv[1], NULL, 10);
    if(m <= 0 || m == LONG_MIN || m == LONG_MAX)
        error_prex("invalid number format");

    int pid = fork();
    if(pid > 0){
	if(execlp("./server", "./server", argv[1], NULL) < 0)
	    error_prex("start server");
    }else if(pid == 0){
	int i;
	for(i = 0; i < m; i++){
	    int client_pid = fork();
	    if(client_pid == 0){
		if(execlp("./client", "./client", NULL) < 0)
		    error_prex("start client");
	    }else if(client_pid < 0){
		error_prex("create client thread");
	    }
	}    
    }else{
	error_prex("create main client thread");
    }
}

void error_prex(char* msg){
    printf("Error: %s, errno: %s\n", msg, strerror(errno));
    exit(-1);
}