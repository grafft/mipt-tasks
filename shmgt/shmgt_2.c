// The program to read process info from shared memory
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_FILE "/bin/ls"

void error_prex(char* msg);
void read_into_str(char** result_str, int fd);

int main(int argc, char** argv, char** envp){

    // generate ipc key
    key_t shm_key = ftok(SHM_FILE, 0);
    if(shm_key < 0)
	error_prex("generate ipc key");
    
    int shmid = shmget(shm_key, 1, 0666);
    if(shmid < 0)
	error_prex("get shared memory");
	
    // attach and print info
    char* info = shmat(shmid, NULL,  SHM_RDONLY);
    if((void*)info == (void*)-1)
	error_prex("attach shared memory");
    printf("Process info (size %d):\n%s", strlen(info), info);
    
    // detach and delete shared memory
    if(shmdt(info) < 0)
	error_prex("detach shared memory");
    if(shmctl(shmid, IPC_RMID, NULL) < 0)
	error_prex("delete shared memory");
	
    return 0;
}

void error_prex(char* msg){
    printf("Error: %s, errno: %s\n", msg, strerror(errno));
    exit(-1);
}
