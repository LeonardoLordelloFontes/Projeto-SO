#ifndef QUEUE_H
#define QUEUE_H

typedef struct {
    int transformations[7];
    char request[256];
    int priority;
    int request_id;
    char* request_pid;
} Task;

typedef struct {
    Task* task_data;
    int tasks;
    int size;
} PriorityQueue;

void initQueue(PriorityQueue q);
void enqueue(PriorityQueue q, Task task);
Task dequeue(PriorityQueue q);
int* peak_transformations(PriorityQueue q);
int isEmpty(PriorityQueue q);

#endif