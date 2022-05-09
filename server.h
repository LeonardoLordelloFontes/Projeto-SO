#ifndef SERVER_H
#define SERVER_H
#include "queue.h"

PriorityQueue q;

typedef struct RunningTask {
    Task task;
    struct RunningTask *next;
} *RunningTasks; 

#endif