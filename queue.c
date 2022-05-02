#include <stdlib.h>
#include <string.h>
#include "queue.h"
#include "server.h"

void swap_task(Task t[], int pos1, int pos2) {
    Task aux = t[pos1];
    t[pos1] = t[pos2];
    t[pos2] = aux;
}

void bubbleUp (Task t[], int i) {
    int p = (i-1)/2;
    while (i > 0 && t[i].priority > t[p].priority || (t[i].priority == t[p].priority && t[i].request_id < t[p].request_id)) {
        swap_task(t, i, p);
        i = p;
        p = (i-1)/2;
    }
}

void bubbleDown (Task t[], int i, int N) {
    int c = 2*i + 1;
    while (c < N && (t[c].priority >= t[i].priority || t[c+1].priority >= t[i].priority)) {
        if (c + 1 < N && (t[c + 1].priority > t[c].priority || (t[c + 1].priority == t[c].priority && t[c + 1].request_id < t[c].request_id)))
            c++;
        swap_task(t, i, c);
        i = c; 
        c = 2*i + 1;
    }
}

void initQueue() {
    q.size = 8;
    q.tasks = 0;
    q.task_data = malloc(sizeof(Task) * q.size);
}

void enqueue(Task task) {
    if (q.tasks == q.size) {
        q.size *= 2;
        q.task_data = realloc(q.task_data, sizeof(Task) * q.size);
    }
    q.task_data[q.tasks] = task;
    bubbleUp(q.task_data, q.tasks);
    q.tasks++;
}

Task dequeue() {
    Task t = q.task_data[0];
    q.tasks--;
    swap_task(q.task_data, 0, q.tasks);
    bubbleDown(q.task_data, 0, q.tasks);
    return t;
}

int* peak_transformations() {
    return q.task_data[0].transformations;
}

int isEmpty() {
    return q.tasks == 0;
}