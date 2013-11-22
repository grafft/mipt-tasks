//Program to write a file with directory file list
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define STD_OUT 1

int main(int argc, char** argv, char** envp){
    // check number of arguments
    if(argc < 2){
	printf("Invalid number of arguments. Usage: %s <directory>\n", argv[0]);
	exit(-1);
    }
    
    // close standart out to use it in future in ls
    if(close(STD_OUT) < 0){
	printf("Error: close stdout\n");
	exit(-1);
    }
    
    // open or create a file and replace descriptor 1 by descriptor of this file
    int fd;
    umask(0);
    if((fd = open("list.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0){
	printf("Error: open file list.txt\n");
	exit(-1);
    }
    
    // write first string to file
    char* first_str = "List of file:\n";
    if(write(fd, first_str, strlen(first_str)) < 0){
	printf("Error: write first string\n");
	exit(-1);
    }
    
    // execute ls with the file replaced stdout
    if(execlp("/bin/ls", "/bin/ls", "-al", argv[1], (char*)NULL) < 0){
	if(close(fd) < 0)
	    printf("Cascade error: close fd\n");
	printf("Error: exec ls\n");
	exit(-1);
    }
}