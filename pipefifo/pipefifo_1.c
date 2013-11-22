// The program to write data about files in directory to pipe and then to fifo
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fcntl.h>

#include <errno.h>
#include <string.h>

#define BUF_SIZE 16
#define FIFO_NAME "list.fifo"

void error_prex(char* msg);
void write_from_str(char* str, int fd);
void read_into_str(char** result_str, int fd);
void parent_work(int* pipe_fd);
void child_work(int* pipe_fd);

int main(int argc, char** argv, char** envp){
    // create pipe
    int pipe_fd[2];
    if(pipe(pipe_fd) < 0)
	error_prex("create pipe");
    
    // create new process
    int child_pid;
    if((child_pid = fork()) > 0){
	parent_work(pipe_fd);
    }else if(child_pid == 0){
	child_work(pipe_fd);
    }else
	error_prex("create process");
    
    return 0;
}

void parent_work(int* pipe_fd){
    if(close(pipe_fd[0]) < 0)
	error_prex("parent close pipe_fd[0]");
	
    // open file with directory file list
    int fd;
    if((fd = open("list.txt", O_RDONLY)) < 0)
	error_prex("open list.txt");

    // read list of files    
    char* result_str = NULL;
    read_into_str(&result_str, fd);
    if(close(fd) < 0)
	error_prex("parent close fd");

    // write to pipe
    write_from_str(result_str, pipe_fd[1]);
	
    if(close(pipe_fd[1]))
	error_prex("parent close pipe_fd[1]");
	
    free(result_str);
}

void child_work(int* pipe_fd){
    if(close(pipe_fd[1]) < 0)
	error_prex("child close pipe_fd[1]");
	
    // read from pipe
    char* result_str = NULL;
    read_into_str(&result_str, pipe_fd[0]);

    // create fifo
    if(mknod(FIFO_NAME, S_IFIFO | 0666, 0) < 0)
	printf("Warning: create fifo file, errno: %s\n", strerror(errno));
    int fifo_fd;
    if((fifo_fd = open(FIFO_NAME, O_WRONLY)) < 0)
	error_prex("open fifo to write");

    // write into fifo
    write_from_str(result_str, fifo_fd);
	
    if(close(pipe_fd[0]))
	error_prex("child close pipe_fd[0]");
	
    free(result_str);
}

void write_from_str(char* str, int fd){
    int size = strlen(str);
    int writed = 0, total_writed = 0;
    do{
	writed = write(fd, str + total_writed, size - total_writed);
	total_writed += writed;
    }while(total_writed < size && writed > 0);
    
    if(writed < 0)
	error_prex("write to fifo");
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