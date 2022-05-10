#ifndef SERVER_H
#define SERVER_H
#include "queue.h"

PriorityQueue q;

typedef struct RunningTask {
    Task task;
    struct RunningTask *next;
} *RunningTasks, runningTask;

Task remove_running_task(pid_t pid);
void sigchld_handler(int sig);
void add_running_task(Task task);
int get_transformation_key(char* str);
int get_task_priority(const char *request); 
int check_transformations_availableness(int* user_transformations); 
void add_user_transformations(int user_transformations[7]); 
void remove_user_transformations(int user_transformations[7]); 
int accept_user_request(int user_transformations[7]); 
int skip_request_priority(char *request); 
int skip_request_to_transformations(char *request); 
void fill_user_transformations(char *request, int user_transformations[7]); 
void status_task(); 
int get_number_of_transformations(int user_transformations[7]); 
void process_transformations(Task task, int number_of_transformations); 
void proc_file_task(char *request, int request_id, char *request_pid);
void select_task(char *request, int request_id); 
void max_runnable_transformations(char* config_path); 
ssize_t read_request(int fd, char* request, size_t size); 

#endif