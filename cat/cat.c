#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define SHELL_NAME "SHELL"

char* find_shell_path(char** envp);

int main(int argc, char** argv, char** envp){
	// check number of arguments
	if(argc < 3){
	    printf("Invalid number of arguments. Usage: %s <directory> <result_file>\n", argv[0]);
	    exit(-1);
	}
	
	// print PCB info
	printf("PID=%d, PPID=%d, UID=%d, GID=%d\n", getpid(), getppid(), getuid(), getgid());
	
	// create new process
	pid_t pid;
	if((pid=fork()) > 0){
	    // int parent, print list of files of directory
	    if(execlp("ls", "ls", argv[1], (char *)NULL) < 0){
		printf("Error exec ls\n");
		exit(-1);
	    }
	}else if(pid == 0){
	    // in child, concatinate all txt files from directory to result file
	    
	    // create command for sh program
	    char* cat_command = "cat ";
	    char* template_str = "/*.txt";
	    char* redirect_str = " > ";
	    size_t size = (strlen(cat_command) + strlen(argv[1]) + strlen(template_str) + strlen(redirect_str) + strlen(argv[2]))*sizeof(char);
	    char* sh_arg = (char*)malloc(size);
	    strcat(sh_arg, cat_command);
	    strcat(sh_arg, argv[1]);
	    strcat(sh_arg, template_str);
	    strcat(sh_arg, redirect_str);
	    strcat(sh_arg, argv[2]);
	    
	    char* shell_path = find_shell_path(envp);
	    // execute sh program with command
	    if(execlp(shell_path, shell_path, "-c", sh_arg, (char*) NULL) < 0){
		free(sh_arg);
		printf("Eror exec cat\n");
		exit(-1);
	    }
	}else{
	    printf("Error fork\n");
	    exit(-1);
	}
}

char* find_shell_path(char** envp){
    int i = 0;
    while(envp[i] != NULL){
	if(strstr(envp[i], SHELL_NAME) != NULL){
	    return strstr(envp[i], "=") + 1;
	}
    }
    return NULL;
}
