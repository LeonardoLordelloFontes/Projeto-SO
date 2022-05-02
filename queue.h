#ifndef QUEUE_H
#define QUEUE_H

typedef struct {
    int transformations[7];
    char request[256];
    int priority;
    int request_id;
} Task;

typedef struct {
    Task* task_data;
    int tasks;
    int size;
} PriorityQueue;

#endif