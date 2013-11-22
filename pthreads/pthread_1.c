// Program to generate matrix and write them to data file
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>

#include <string.h>
#include <errno.h>
#include <limits.h>

void error_prex(char* msg);
char* ltostr(long num);
void write_from_str(char* str, int fd);

char* generate_matrix(long n);

#define NUM_START_INDEX 48
#define MAX_NUM 1000
#define GEN_BUF 1024

int main(int argc, char** argv, char** envp){
    // check input params
    if(argc < 2){
	printf("Invalid number of arguments. Usage: %s <matrix_size>\n", argv[0]);
        exit(-1);
    }

    // convert input str to number
    long n = strtol(argv[1], NULL, 10);
    if(n <= 0 || n == LONG_MIN || n == LONG_MAX)
	error_prex("invalid number format");

    // open data file
    umask(0);
    int fd = open("data.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd < 0)
	error_prex("open or create file");
	
    // generate and write both matrixes
    write_from_str(argv[1], fd);
    write_from_str("\n", fd);
    
    srand(time(NULL));
    char* matrix1 = generate_matrix(n);
    write_from_str(matrix1, fd);
    free(matrix1);

    char* matrix2 = generate_matrix(n);
    write_from_str(matrix2, fd);
    free(matrix2);

    if(close(fd) < 0)
	error_prex("close data file");
	
    return;
}

void write_from_str(char* str, int fd){
    int size = strlen(str);
    int writed = 0, total_writed = 0;
    do{
	writed = write(fd, str + total_writed, size - total_writed);
	total_writed += writed;
    }while(total_writed < size && writed > 0);
    
    if(writed < 0)
	error_prex("write to file");
}

char* generate_matrix(long n){
    int i, resize_count = 1, total_count = 0;
    char* matrix = (char*)calloc(GEN_BUF, sizeof(char));
    for(i = 0; i < n * n; i++){
        // generate random number and convert it to str
        char* num_str = ltostr(rand() % MAX_NUM);
        int added_size = strlen(num_str) + 1;
        if(total_count + added_size + 1 >= GEN_BUF){
	    resize_count += 1;
	    matrix = (char*)realloc(matrix, resize_count * GEN_BUF * sizeof(char));
	    total_count = total_count + added_size - GEN_BUF;
	}else{
	    total_count += added_size;
	}
	
	strcat(matrix, num_str);
	if(i == (n * n - 1))
	    strcat(matrix, "\n");
	else
	    strcat(matrix, " ");
	free(num_str);
    }
    int result_len = (resize_count - 1) * GEN_BUF + total_count;
    char* result = (char*)malloc(result_len * sizeof(char));
    strncpy(result, matrix, result_len);
    return result;
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