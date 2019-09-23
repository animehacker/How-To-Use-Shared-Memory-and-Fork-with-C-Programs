#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "vector.h"
#include <time.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#define MIN_REQUIRED 2

// Hard coded help page.
void help(){
    printf("Usage: consumer [OPTION]\nDestroy print jobs created by the producer.\n\n  -d, --destroy-jobs    Specify number of jobs to be destroyed per cycle. Number must be over zero.\n      --help     Display this help and exit.\n      --version  Output version information and exit.\n\n\nExamples:\n  consumer --destroy-jobs 1     Destroy 1 job per cycle.\n  consumer -d 3                 Destroy 3 jobs per cycle.\n\nReport Consumer bugs to cs166530@student.staffs.ac.uk\nGeneral help using this software can be found in the user guide and tutorial video.\n");
    exit(1);
}
// Print version information
void printVersion(){
    printf("Consumer version 1.0\n");
    exit(1);
}
// Function to return current date/time
char getDateTime(){
            char currentDateTime[20];
            struct tm *sTm;
            time_t now = time (0);
            sTm = gmtime (&now);
            strftime (currentDateTime, sizeof(currentDateTime), "%Y-%m-%d %H:%M:%S", sTm);
            return currentDateTime;
}
// Check if an integer is valid and if not return false. If valid then assign the inputted value to a variable in the program.
bool isInt(/* in */ char *string,/* out */ int *num){
    char *endp;
    long n;
    n = strtol(string, &endp, 0);
    if(!*endp && !errno && INT_MIN <= n && n <= INT_MAX) {
        *num = (int)n;
        return true;
    }
    return false;
}
// Function to log events.
void logEvent(char *event){
    FILE *f = fopen("access.log", "a+");
    if (f == NULL) printf("Error opening log file! Have you checked permissions?\n");
    fprintf(f, event);
    fclose(f);
}
void killJobs(struct vector *array,int max){
    int i = 0;
    int j = 0;
    int highestPriority = 0;
    int tempIndex; // Store the index location of the highest priority job in the array.
    char currentDateTime[20];
    struct tm *sTm;
    time_t now = time (0);
    sTm = gmtime (&now);
    strftime (currentDateTime, sizeof(currentDateTime), "%Y-%m-%d %H:%M:%S", sTm);
    for (i;i < array[0].killPerCycle;i++){
            for (j;j < max;j++){ 
                if(highestPriority < array[j].priority){
                     highestPriority = array[j].priority;
                     tempIndex = j;
                }
            }
            system("kill -9 "+array[tempIndex].pid);
            printf("%s: Job with process ID %d and priority %d has been killed.\n",currentDateTime,array[tempIndex].pid,array[tempIndex].priority);
            char *event[32];
            //strcat(event,);
            snprintf(event, sizeof event, "%s, [102], Job destroyed with PID %d and priority %d\n", currentDateTime,array[tempIndex].pid,array[tempIndex].priority);
            logEvent(event);
            array[tempIndex].priority = 0; // Set priority to zero to denote a killed process/ job. 
            array[tempIndex].pid = 0; // Set PID to 0 for the same reason. 
            j=0;
            highestPriority = 0;
        }
        return;
}
int readSharedSize()
{
        int   shmid;
        int  *shm, *s;
	int SHMSZ = sizeof(int);
        key_t key =  6954349;
        if((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0)
        {
                printf("Consumer process has run into an error while trying to attach to a shared memory segment. Please run the producer process first.\n");
                system("./cleansharedmem >/dev/null 2>&1");
                exit(1);
        }
        if((shm = shmat(shmid, NULL, 0)) == (int *)-1)
        {
                printf("Consumer process has run into an error while trying to attach to a shared memory segment. Please run the producer process first.\n");
                system("./cleansharedmem >/dev/null 2>&1");
                exit(1);
        }
	int *returnSize = malloc(sizeof(int));
	*returnSize = *shm;
        if (*returnSize > 10000){
                 printf("Return size is %d\n",*returnSize);
                 printf("Consumer process has run into an error while trying to attach to a shared memory segment. Please run the producer process first.\n");
                 system("./cleansharedmem >/dev/null 2>&1");
                 exit(1);
        }
        return *returnSize;
}
struct vector *readSharedMem(int x){
int shmid;
struct vector *array;

int i = 0;
key_t key =  6954346;

if((shmid = shmget(key, x*sizeof(struct vector), IPC_EXCL)) < 0)
        {
                printf("Consumer process has run into an error while trying to attach to a shared memory segment. Please run the producer process first.\n");
                system("./cleansharedmem >/dev/null 2>&1");
                exit(1);
        }

if((array = shmat(shmid, 0, 0))  == (int *)-1)
        {
                printf("Consumer process has run into an error while trying to attach to a shared memory segment. Please run the producer process first.\n");
                system("./cleansharedmem >/dev/null 2>&1");
                exit(1);
        }

    return array;
}

int main(int argc, char *argv[]){
    // Initialise variables
    extern char *__progname;
    int i;
    FILE *fp;
    char procExists[1024];
    char rootDir[512];
    char *expectedPath[1024];
    char *realPath[1024];
    // Retreive current working directory
    fp = popen("pwdx `pgrep consumer` 2>/dev/null | head -n 1 |  awk '{print $2}' | tr -d '\n'", "r");
    fgets(rootDir, sizeof(rootDir)-1, fp);
    snprintf(expectedPath, sizeof expectedPath, "%s/consumer",rootDir);
    snprintf(realPath, sizeof realPath, "%s/%s",rootDir,__progname);
    // Check if process is already running. If so then exit, multiple instances of Producer may cause Process ID conflicts, shared memory permission issues and general system instability.
    pclose(fp);
    fp = popen("/bin/ps -A | grep consumer", "r");
    if (fp == NULL) {
    printf("Failed to run command. An error has occured and consumer has exited.\n" );
    exit(1);
    }
    while (fgets(procExists, sizeof(procExists)-1, fp) != NULL) {
      //printf("%s", procExists); // Kept this line for debugging purposes.
      i++;
    }
    pclose(fp);
    if(i!=1){
         printf("It seems you are already running this process. Please kill all instances of consumer with killall -9 consumer. You may need to run this command with Sudo.\n");
         exit(1);
        }
    i=0;
    if(!(strcmp(argv[0], "producer") == 0) && !(strcmp(argv[0], "./consumer") == 0) && !(strcmp(realPath, expectedPath) == 0)){
        printf("This process should be named consumer before executing.\n");
        exit(1);
    }
    // Check if minimum required parameters entered.
    if (argc < 2) help();
    else if(strcmp(argv[1], "--help") == 0) help();
    else if(strcmp(argv[1], "--version") == 0) printVersion();
    else if(argc< MIN_REQUIRED) help();
    else if(strcmp(argv[1], "--destroy-jobs") != 0 && strcmp(argv[1], "-d") != 0) help();
    int killPerCycle, lastCycleCount, totalKilled;
    if (!isInt(argv[2],&killPerCycle)) help();
    else if (killPerCycle==0) help();
    struct vector *array;
    int maxJobs = readSharedSize();
    if(maxJobs < killPerCycle){
            printf("Warning! Destroy jobs value must be less than or equal to max jobs value in Producer. Producer must also be running for this check to pass. Please read the help file below.\n\n");
	    system("./cleansharedmem >/dev/null 2>&1");
            help();
    }
    array = readSharedMem(maxJobs);
    while(true){    
             lastCycleCount = array[0].cycleCount; // Get the current cycle count;
             printf("\nWaiting for cycle to finish..\n\n");
             while (array[0].cycleCount==lastCycleCount){ 
                     sleep(1);
                     }
             if (killPerCycle <= array[0].jobsAlive){
                     array[0].killPerCycle = killPerCycle;
                     printf("Killing %d jobs on cycle number %d:\n",killPerCycle,array[0].cycleCount);
                     killJobs(array,maxJobs);
                     totalKilled+=killPerCycle;
                        }
             else{
                     array[0].killPerCycle = 0;
                     printf("Consumer thinks you are trying to kill more jobs than exist. Consumer will not kill jobs for this cycle.");
             }
             }
    return 0;
}
