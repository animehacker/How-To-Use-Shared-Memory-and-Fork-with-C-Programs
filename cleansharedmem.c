#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "vector.h"
#include "vector.c"
#include "bool.c"

// Hard coded help page.
void help(){
    printf("Usage: cleansharedmem\n\nRun without any parameters given to detach any shared memory segments used by the Producer and Consumer processes clearing up any potential runtime errors you may face with this program.\n\nThese errors can occur when a process is killed prematurely without detaching from shared memory segments, locking out write access to the segments.\n");
    exit(1);
}
int readSharedSize()
{
        int   shmid;
        int  *shm, *s;
	int SHMSZ = sizeof(int);
        key_t key =  6954349;
        if((shmid = shmget(key, SHMSZ, 0666)) < 0)
        {
                printf("There was an error. Most likely the memory segment has already been detached and you don't need to run this process.\n\nTry running the Producer process first and then the Consumer process afterwards.\n");
                exit(1);
        }
        if((shm = shmat(shmid, NULL, 0)) == (int *)-1)
        {
                printf("There was an error. Most likely the memory segment has already been detached and you don't need to run this process.");
                exit(1);
                exit(EXIT_FAILURE);
        }
        int *returnSize = malloc(sizeof(int));
	*returnSize = *shm;
          shmctl(shmid, IPC_RMID, NULL);
          printf("Deleted SHM key 6954349.\n");
          return *returnSize;
}
struct vector *readSharedMem(int x){
int shmid;
struct vector *array;
int i = 0;
key_t key =  6954346;
shmid = shmget(key, x*sizeof(struct vector), IPC_EXCL);
array = shmat(shmid, 0, SHM_RDONLY);
    shmctl(shmid, IPC_RMID, NULL);
    printf("Deleted SHM key 6954346.\n\n If you were having segmentation fault errors running either the Producer or Consumer process then they should be solved now. This happens when a process is killed prematurely without detaching from a shared memory segment.\n");
    return array;
}
int main(int argc, char *argv[]){
        if(argc > 1 && strcmp(argv[1], "--help") == 0) help();
        struct vector *array;
        int size = readSharedSize();
        array = readSharedMem(size);
}
