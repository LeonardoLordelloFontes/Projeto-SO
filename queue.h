#ifndef QUEUE_H
#define QUEUE_H

typedef struct {
    int transformations[7];
    char request[256];
    int priority;
    int request_id;
    int request_fd;
    pid_t child_process;
} Task;

typedef struct {
    Task* task_data;
    int tasks;
    int size;
} PriorityQueue;

void initQueue();
void enqueue(Task task);
Task dequeue();
int* peak_transformations();
int isEmpty();

#endif