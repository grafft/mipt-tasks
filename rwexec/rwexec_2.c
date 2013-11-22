//Program to read a file with directory file list
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define STD_OUT 1
#define BUF_SIZE 16

int main(int argc, char** argv, char** envp){
    // open file to read
    int fd;
    if((fd = open("list.txt", O_RDONLY)) < 0){
	printf("Error: open file list.txt\n");
	exit(-1);
    }
    
    // create buffer and resulted string
    char* buffer = (char*)malloc(BUF_SIZE * sizeof(char));
    char* result_str = NULL;
    
    // read into buffer and populate resulted string
    int readed = 0;
    while((readed = read(fd, buffer, BUF_SIZE)) > 0){
	if(result_str == NULL){
	    result_str = (char*)calloc(readed + sizeof(char), 1);
	}else{
	    result_str = realloc(result_str, (strlen(result_str) + 1) * sizeof(char) + readed);
	}
	strncat(result_str, buffer, readed);
    }
    if(readed < 0){
	printf("Error: read file list.txt\n");
	exit(-1);
    } 

    // print result
    printf("%s", result_str);
    free(buffer);
    free(result_str);
    
    if(execlp("/bin/rm", "/bin/rm", "list.txt", (char*)NULL) < 0){
	printf("Error: exec rm\n");
	exit(-1);
    }
}