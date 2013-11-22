// Program to write process data to shared memory
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define BUF_SIZE 16
#define SHM_FILE "/bin/ls"

void error_prex(char* msg);
void read_into_str(char** result_str, int fd);

int main(int argc, char** argv, char** envp){
    // check number of arguments
    if(argc < 2){
        printf("Invalid number of arguments. Usage: %s <pid>\n", argv[0]);
        exit(-1);
    }
    
    // create pipe
    int pipe_fd[2];
    if(pipe(pipe_fd) < 0)
	error_prex("create pipe");
    
    int child_pid;
    if((child_pid = fork()) == 0){
	// in child exec ps with replaced stdout by pipe
	if(close(pipe_fd[0]) < 0)
	    error_prex("close child pipe fd[0]");
	if(dup2(pipe_fd[1], 1) < 0)
	    error_prex("duplicate pipe fd to stdout");
	execlp("/bin/ps", "/bin/ps", "u", "-p", argv[1], (char*)NULL);
    }else if(child_pid > 0){
	// in parent read info from pipe from ps
	if(close(pipe_fd[1]) < 0)
	    error_prex("close parent pipe fd[1]");
	    
	char* result_str = NULL;
	read_into_str(&result_str, pipe_fd[0]);
	if(close(pipe_fd[0]) < 0)
	    error_prex("close parent pipe fd[0]");
	
	// create shared memory
	key_t shm_key = ftok(SHM_FILE, 0);
	if(shm_key < 0)
	    error_prex("generate ipc key");
	int shmid = shmget(shm_key, strlen(result_str), IPC_CREAT | IPC_EXCL | 0666);
	if(shmid  < 0)
	    error_prex("create shared memory");
	
	// attach shared memory
	char* shared = (char*)shmat(shmid, NULL, 0);
	if((void*)shared  == (void*)-1)
	    error_prex("attach shared memory");

	// cope data to shared memory
	strcpy(shared, result_str);
	
	// free resources
	free(result_str);
	if(shmdt(shared) < 0)
	    error_prex("detach sahred memory");

    }else
	error_prex("fork process");
}

void read_into_str(char** result_str, int fd){
    // create buffer and resulted string
    char* buffer = (char*)malloc(BUF_SIZE*sizeof(char));
        
    // read into buffer and populate resulted string
    int readed = 0;
    while((readed = read(fd, buffer, BUF_SIZE)) > 0){
        if(*result_str == NULL){
            *result_str = (char*)calloc(readed + sizeof(char), 1);
        }else{
            *result_str = realloc(*result_str, (strlen(*result_str) + 1) * sizeof(char) + readed);
        }
	strncat(*result_str, buffer, readed);
    }
    if(readed < 0)
        error_prex("read from file");

    free(buffer);
}

void error_prex(char* msg){
    printf("Error: %s, errno: %s\n", msg, strerror(errno));
    exit(-1);
}
