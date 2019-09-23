#ifndef VECTOR_H
#define VECTOR_H
#define VECTOR_INIT_CAPACITY 4

typedef struct vector {
    struct vector *next;
    int priority;
    int pid;
    int cycleCount; // Only use on first element of the array.
    int killPerCycle; // Only use on first element of the array.
    int jobsAlive; // Only use on first element of the array.
    int SHMID; // Only use on first element of the array.
} vector;
#endif