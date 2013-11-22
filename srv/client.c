#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>

#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define BUF_SIZE 16
#define LOG_FILE "log.txt"
#define MAX_MSG_SIZE 256
#define IPC_GEN_FILE "/bin/ls"
#define NUM_START_INDEX 48

#define FIRST_MSG_ID 1
#define LAST_MSG_ID 2

struct client_msg{
    long mtype;
    int data[MAX_MSG_SIZE];
};

int* recieve_matrix(int msgid, int msgtype);
void write_log(int semid, int logfd, char* log_msg);

void error_prex(char* msg);
void write_from_str(char* str, int fd);

char* ltostr(long num);

int semid;
int logfd;

int main(int argc, char** argv, char** envp){
    // create or get message quewe
    key_t msg_key = ftok(IPC_GEN_FILE, 1);
    if(msg_key < 0)
        error_prex("generate IPC shm key");
    int msgid = msgget(msg_key, 0666 | IPC_CREAT);
    if(msgid < 0)
        error_prex("create or get message quewe");
	
    // open or create log file
    logfd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if(logfd < 0)
	error_prex("create or open log file");
    
    // create or get semafors for log
    key_t sem_key = ftok(IPC_GEN_FILE, 2);
    if(sem_key < 0)
	error_prex("generate IPC sem key");
    semid = semget(sem_key, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if(semid < 0 && errno == EEXIST){
	if((semid = semget(sem_key, 1, 0)) < 0)
	    error_prex("get semafors");
    }else if(semid >= 0){
	struct sembuf sem_oper;
	sem_oper.sem_num = 0;
	sem_oper.sem_flg = 0;
	sem_oper.sem_op = 1;
	semop(semid, &sem_oper, 1);
	
	write_log(semid, logfd, "Create semaphor");
    }else
	error_prex("create semaphors");
    
    // send hello message
    write_log(semid, logfd, "Send hello message to server");
    struct client_msg hello_msg;
    hello_msg.mtype = FIRST_MSG_ID;
    hello_msg.data[0] = getpid();
    if(msgsnd(msgid, (struct msgbuf*) &hello_msg, sizeof(int), 0) < 0)
	error_prex("send hello message");
	
    int* matrix1 = recieve_matrix(msgid, getpid());
    
    if(semctl(semid, 0, IPC_RMID, NULL) < 0)
	error_prex("delete semafors");
}

int* recieve_matrix(int msgid, int msgtype){
    struct client_msg msg_init;
    if(msgrcv(msgid, (struct msgbuf*) &msg_init, 2 * sizeof(int), msgtype, 0) < 0)
        error_prex("get size message");
    write_log(semid, logfd, "Recieve size of work");
    
    int size = msg_init.data[0];
    int work_size = msg_init.data[1];
    
    struct client_msg msg_data;
    msg_data.mtype = msgtype;
    int* matrix = (int*)malloc(work_size * size * sizeof(int));
    int i, recieved = 0, num_packets = (work_size * size * sizeof(int)) / MAX_MSG_SIZE + 1;
    for(i = 0; i < num_packets; i++){
	int rec_size = (i == (num_packets - 1)) ? (work_size * size) * sizeof(int) - recieved : MAX_MSG_SIZE;
        if(msgrcv(msgid, (struct msgbuf*) &msg_data, MAX_MSG_SIZE, msgtype, 0) < 0)
    	    error_prex("get data from server");
    	memcpy(matrix + i * MAX_MSG_SIZE, msg_data.data, rec_size);
        recieved += rec_size;
    }

    write_log(semid, logfd, "Recieved work");
}

void write_log(int semid, int logfd, char* log_msg){
    char* prefix = "[Client ";
    char* client_id = ltostr(getpid());
    char* suffix = ": ";
    
    //time_t seconds = time(NULL);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* date_time = localtime(&tv.tv_sec);
    char* format = "%d.%m.%y %H:%M:%S.";
    int size = strlen(format) + 6;
    char* date = (char*)malloc((size + 1) * sizeof(char));
    strftime(date, size, format, date_time);
    char* microsecs = ltostr(tv.tv_usec % (1000 * 1000));
    char* postfix = "] ";
    char* postpostfix = "\n";
    char* str_log = (char*)malloc((strlen(log_msg) + strlen(prefix) + strlen(client_id) + strlen(suffix) + 
	strlen(date) + strlen(microsecs) + strlen(postfix) + strlen(postpostfix)) * sizeof(char));
    str_log[0] = 0;
    
    strcat(str_log, prefix);
    strcat(str_log, client_id);
    strcat(str_log, suffix);
    strcat(str_log, date);
    strcat(str_log, microsecs);
    strcat(str_log, postfix);
    strcat(str_log, log_msg);
    strcat(str_log, postpostfix);
    
    struct sembuf sem_oper;
    sem_oper.sem_num = 0;
    sem_oper.sem_flg = 0;
    
    sem_oper.sem_op = -1;
    if(semop(semid, &sem_oper, 1) < 0)
	error_prex("decrease semafor");
    write_from_str(str_log, logfd);
    sem_oper.sem_op = 1;
    if(semop(semid, &sem_oper, 1) < 0)
	error_prex("increase cemafor");
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

char* ltostr(long num){
    int has_min = num < 0;
    long tmp = has_min ? -num : num;
    
    // count number of digits
    int number_of_digit = 1;
    while((tmp = tmp/10) > 0)
	number_of_digit++;

    // allocate size for digits, minus and 0
    int start = 0;
    int size_with_0 = number_of_digit + (has_min ? 1 : 0) + 1;
    char* result = (char*)calloc(size_with_0, sizeof(char));
    if(has_min){
	result[0] = '-';
	start = 1;
    }
    
    long pow = 1;
    tmp = has_min ? -num : num;
    int i;
    // write digits from end
    for(i = 0; i < number_of_digit; i++){
	int digit = (tmp - (tmp / (pow * 10)) * pow * 10) / pow;
	result[number_of_digit - 1 + start - i] = NUM_START_INDEX + digit;
	pow = pow * 10;
    }
    result[size_with_0 - 1] = 0;
    
    return result;
}

void error_prex(char* msg){
    printf("Error: %s, errno: %s\n", msg, strerror(errno));
    exit(-1);
}