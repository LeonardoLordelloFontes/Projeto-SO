#ifndef QUEUE_H
#define QUEUE_H

#define MAX_PRIORITY 5

typedef struct User {
    int transformations[7];
    
};

typedef struct Node {
    struct User;
    struct Queue_Node *next;
} *NodePtr;

typedef struct Queue {
    NodePtr front;
    NodePtr end;
} *QueuePtr;

typedef QueuePtr PriorityQueue[MAX_PRIORITY];


#endif