#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>

#include <string.h>
#include <errno.h>
#include <limits.h>

#define NUM_START_INDEX 48
#define BUF_SIZE 16

struct thread_data{
    int* matrix1;
    int* matrix2;
    int start_index; //include to calculate
    int end_index; // exlude from calculate
    int size;
};

void error_prex(char* msg);
void write_from_str(char* str, int fd);
void read_into_str(char** result_str, int fd);

char* ltostr(long num);
char* mtostr(int* matrix, int size);
int read_matrix(int** matrix, int size, char* src_str, int start_index);
void* thread_work(void* arg);

int main(int argc, char** argv, char** envp){
    if(argc < 2){
	printf("Invalid number of arguments. Usage: %s <thread_count>\n", argv[0]);
        exit(-1);
    }
    int m = strtol(argv[1], NULL, 10);
    if(m <= 0 || m == LONG_MIN || m == LONG_MAX)
	error_prex("invalid number format");
	
    // read matrixes
    int fd = open("data.txt", O_RDONLY);
    if(fd < 0)
	error_prex("open data file");
	
    // read matrixes string
    char* matrixes = NULL;
    read_into_str(&matrixes, fd);
    
    if(close(fd) < 0)
	error_prex("close data file");
    
    // extract size of matrixes
    int i = 0;
    while(matrixes[i] != '\n'){
	i++;
    }
    char* tmp = (char*)calloc(i + 1, sizeof(char));
    strncpy(tmp, matrixes, i);
    int size = strtol(tmp, NULL, 10);
    free(tmp);
    
    // read both matrixes
    int* matrix1 = (int*)malloc((size*size - 1) * sizeof(int));
    i = read_matrix(&matrix1, size, matrixes, i + 1);
    int* matrix2 = (int*)malloc((size * size - 1) * sizeof(int));
    read_matrix(&matrix2, size, matrixes, i);
    
    
    // define work for one thread
    int work_len = 1;
    int work_num = m;
    if(m < size)
	work_len = size / m;
    else
	work_num = size;
    
    // start timer
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    
    // create and start threads
    pthread_t thd_ids[work_num];
    struct thread_data datas[work_num];
    for(i = 0; i < work_num; i++){
	datas[i].matrix1 = matrix1;
	datas[i].matrix2 = matrix2;
	datas[i].start_index = i * work_len;
	if(i == work_num - 1)
	    datas[i].end_index = size;
	else
	    datas[i].end_index = (i + 1) * work_len;
	datas[i].size = size;
	if(pthread_create(&thd_ids[i], NULL, thread_work, &datas[i]) > 0)
	    error_prex("create thread");
    }

    // collect results from threads
    int* result = (int*)malloc((size*size - 1) * sizeof(int));
    for(i = 0; i < work_num; i++){
	int** tmp = (int**)malloc(sizeof(int*));
	if(pthread_join(thd_ids[i], (void**)tmp) > 0)
	    error_prex("collect thread result");
	int j,k;
	for(j = 0; j < (datas[i].end_index - datas[i].start_index); j++)
	    for(k = 0; k < size; k++)
		result[((datas[i].start_index + j) * size) + k] = (*tmp)[j * size + k];
	free(*tmp);
	free(tmp);
    }
    struct timeval end_time; 
    gettimeofday(&end_time,NULL);
    int delta = end_time.tv_usec - start_time.tv_usec;
    int sec = delta / (1000 * 1000);
    int milsec = (delta - sec * 1000 * 1000) / 1000;
    int micsec = delta - sec * 1000 * 1000 - milsec * 1000;
    printf("Counting time: %ds %dms %dmcs\n", sec, milsec, micsec);
    
    // convert integer matrix to string    
    char* result_str = mtostr(result, size);

    // open result file
    umask(0);
    fd = open("result.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd < 0)
        error_prex("open or create result file");

    write_from_str(result_str, fd);
    free(result_str);
    if(close(fd) < 0)
	error_prex("close result file");
}

void* thread_work(void* arg){
    struct thread_data* data = (struct thread_data*)arg;
    int work_len = data->end_index - data->start_index;
    int* result = (int*) malloc((work_len * data->size) * sizeof(int));
    
    int i,j,k;
    for(i = 0; i < work_len; i++)
	for(j = 0; j < data->size; j++)
	    for(k = 0; k < data->size; k++)
		result[i * data->size + j] += data->matrix1[(data->start_index + i) * data->size + k] * data->matrix2[k * data->size + j];
    
    return result;
}

char* mtostr(int* matrix, int size){
    char* result_str = NULL;
    int i;
    for(i = 0; i < size*size; i++){
	char* tmp = ltostr(matrix[i]);
	if(result_str == NULL){
	    result_str = (char*)calloc(strlen(tmp) + 1, sizeof(char));
	}else{
	    int old_size = strlen(result_str);
	    result_str = (char*)realloc(result_str, (old_size + strlen(tmp) + 2) * sizeof(char));
	    result_str[old_size] = ' ';
	    result_str[old_size + 1] = 0;
	}
	strcat(result_str, tmp);
	free(tmp);
    }
    
    return result_str;
}

int read_matrix(int** matrix, int size, char* src_str, int start_index){
    int counter = 0, index = start_index, digits = 0;
    char* tmp;
    // parse all size * size - 1 integers
    while(counter < size * size){
	//count number of digits
	while(src_str[index + digits] != '\n' && src_str[index + digits] != ' ')
	    digits++;
	    
	// allocate memory for int string with \0
	tmp = (char*)calloc(digits + 1, sizeof(char));
	strncpy(tmp, src_str + index, digits);
	(*matrix)[counter] = strtol(tmp, NULL, 10);
	free(tmp);
	
	index = index + digits + 1;
	digits = 0;
	counter++;
    }
    return index;
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
    char* buffer = (char*)malloc(BUF_SIZE * sizeof(char));
             
    // read into buffer and populate resulted string
    int readed = 0;
    while((readed = read(fd, buffer, BUF_SIZE)) > 0){
	if(*result_str == NULL){
    	    *result_str = (char*)calloc(readed + sizeof(char), 1);
        }else{
            *result_str = realloc(*result_str, (strlen(*result_str) + 1)*sizeof(char) + readed);
        }
        strncat(*result_str, buffer, readed);
    }
    if(readed < 0)
	error_prex("read from file");
	
    free(buffer);
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