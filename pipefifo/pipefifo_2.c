// The program to read data from fifo
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>

#define BUF_SIZE 16
#define FIFO_NAME "list.fifo"

void error_prex(char* msg);
void read_into_str(char** result_str, int fd);

int main(int argc, char** argv, char** envp){
    // open fifo file
    int fifo_fd;
    if((fifo_fd = open(FIFO_NAME, O_RDONLY)) < 0)
	error_prex("open fifo file");
	
    // read from fifo file
    char* result_str = NULL;
    read_into_str(&result_str, fifo_fd);
    printf(result_str);
    
    free(result_str);
    
    // delete fifo file
    if(execlp("/bin/rm", "/bin/rm", FIFO_NAME, (char*) NULL) < 0)
	error_prex("exec rm");
}


void read_into_str(char** result_str, int fd){
    // create buffer and resulted string
    char* buffer = (char*)malloc(BUF_SIZE*sizeof(char));
        
    // read into buffer and populate resulted string
    int readed = 0;
    while((readed = read(fd, buffer, BUF_SIZE)) > 0){
        if(*result_str == NULL){
            *result_str = (char*)malloc(readed);
        }else{
            *result_str = realloc(*result_str, strlen(*result_str)*sizeof(char) + readed);
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
