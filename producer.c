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
#include <sys/types.h>
#include <sys/stat.h>
#define MIN_REQUIRED 6

bool verbose;
// Hard coded help page.
void help(){
    printf("Usage: producer [OPTION]\nCreate print jobs to be consumed by the consumer process.\n\n  -m, --max-jobs           Max number of jobs that can be created before this process and the consumer process are terminated.\n  -j, --jobs-per-cycle     Number of jobs to create per cycle.\n  -l, --log-level          Log level detail from levels 1 to 3. You may want to increase the log level to capture more details.\n  -v, --verbose            Verbose output to the console.\n      --help     Display this help and exit.\n      --version  Output version information and exit.\n\n\nExamples:\n  producer --max-jobs 30 --jobs-per-cycle 6 --log-level 2  Create 6 jobs per cycle up to a total of 5 cycles. Log events in maximum detail.\n  producer -l 1 -m 18 -j 2                                 Create 2 jobs per cycle up to a total of 9 cycles. Log events in minimum detail.\n  producer --jobs-per-cycle 4 -m 48 -l 0                   Create 4 jobs per cycle up to a total of 12 cycles. Do not log events.\n\nReport Producer bugs to cs166530@student.staffs.ac.uk\nGeneral help using this software can be found in the user guide (readme.txt) and the tutorial video - https://www.youtube.com/watch?v=dbT7FAuHBcI.\n");
    exit(1);
}
// Print version information.
void printVersion(){
    printf("Producer version 1.0\n");
    exit(1);
}
// Function to return current date/time.
char *getDateTime(){
            char currentDateTime[20];
            struct tm *sTm;
            time_t now = time (0);
            sTm = gmtime (&now);
            strftime (currentDateTime, sizeof(currentDateTime), "%Y-%m-%d %H:%M:%S", sTm);
            return currentDateTime;
}
// Useful function to check if a file exists.
int file_exist (char *filename)
{
  struct stat   buffer;   
  return (stat (filename, &buffer) == 0);
}
// Function to log events.
void logEvent(char *event,char *fileName){
    FILE *f = fopen(fileName, "a+");
    if (f == NULL) printf("Error opening log file! Have you checked permissions?\n");
    else fprintf(f, event);
    if (verbose==true) printf("Log: %s\n", event); 
    fclose(f);
}
int returnAliveJobs(struct vector *array, int max){
    int i = 0;
    int jobCount = 0;
    for (i;i < max;i++) if (array[i].priority!=0) jobCount++;
    return jobCount;
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
int sendSharedSize(int max){
key_t key =  6954349;
   int  c;
   int   shmid;
   int *shm, *s;
   int SHMSZ = sizeof(int);
   /* Create the segment */
   if((shmid = shmget(key, SHMSZ, IPC_EXCL | IPC_CREAT | 0666)) < 0)
   {
      perror("shmget");
      exit(1);
        }

   /* Now attach the segment to the data space */
        if((shm = shmat(shmid,NULL,0)) == (int *)-1)
        {
                perror("shmat");
                exit(1);
        }
        
    s = shm;
	*s = max;
    return max;
}
struct vector *setSharedMem(int max, int **shmid, int logLevel){
    struct vector *array;
    int count = max;
    int i = 0;
    int SizeMem;
    key_t key = 6954346;

    SizeMem = (sizeof(struct vector)*max);



 if ((shmid = shmget(key, SizeMem, IPC_CREAT | 0666)) == -1) {
            perror("shmget");
            exit(1);
        }

array = (struct vector *)shmat(shmid, 0, 0);

array[0].SHMID = shmid; // Store the SHMID here so we can detach the shared memory segment in main. 
char *event[32];
char *currentDateTime[20];
struct tm *sTm;
time_t now = time (0);
sTm = gmtime (&now);
strftime (currentDateTime, sizeof(currentDateTime), "%Y-%m-%d %H:%M:%S", sTm);
snprintf(event, sizeof event, "%s, [103], Size of memory segment is %d and SHMID is %d\n", currentDateTime,SizeMem,(int)shmid);
//if (logLevel > 1) logEvent(event,"access.log");
return array;
}
bool promptUser(){
    bool quit = false;
    char input;
	     printf("Please press 'q' to exit or any other key to continue: ");
         while ((input=getchar()) != EOF && input != '\n'){
            switch (input){
	        case 'q':
            quit = true;
            break;
            }
        }
        printf("\n");
        return quit;
	    }
main(int argc, char *argv[]){
    extern char *__progname;
    // Let's reserve some variables.
    int i,j,k;
    char *currentDateTime;
    char *event[32];
    FILE *fp;
    char procExists[1024];
    char rootDir[512];
    char *expectedPath[1024];
    char *realPath[1024];
    // If no error log exists then create one with useful headers.
   if (!file_exist ("error.log")) logEvent("Date, Error Key, Event\n\n","error.log");
    // Retreive current working directory
    fp = popen("pwdx `pgrep producer` 2>/dev/null | head -n 1 |  awk '{print $2}' | tr -d '\n'", "r");
    fgets(rootDir, sizeof(rootDir)-1, fp);
    snprintf(expectedPath, sizeof expectedPath, "%s/producer",rootDir);
    snprintf(realPath, sizeof realPath, "%s/%s",rootDir,__progname);
    // Check if process is already running. If so then exit, multiple instances of Producer may cause Process ID conflicts, shared memory permission issues and general system instability.
    pclose(fp);
    fp = popen("/bin/ps -A | grep producer", "r");
    if (fp == NULL) {
    printf("Failed to run command. An error has occured and producer has exited.\n" );
    currentDateTime = getDateTime();
    snprintf(event, sizeof event, "%s, [200], Failed to run command. An error has occured and producer has exited.\n", currentDateTime);
    logEvent(event,"error.log");
    exit(1);
    }
    while (fgets(procExists, sizeof(procExists)-1, fp) != NULL) {
      //printf("%s", procExists); // Kept this line for debugging purposes.
      i++;
    }
    pclose(fp);
    if(i!=1){
         printf("It seems you are already running this process. Please kill all instances of producer with killall -9 producer. You may need to run this command with Sudo.\n");
         currentDateTime = getDateTime();
         snprintf(event, sizeof event, "%s, [201], It seems you are already running this process. Please kill all instances of producer with killall -9 producer. You may need to run this command with Sudo.\n", currentDateTime);
         logEvent(event,"error.log");
         exit(1);
        }
    i=0;
    if(!(strcmp(argv[0], "producer") == 0) && !(strcmp(argv[0], "./producer") == 0) && !(strcmp(realPath, expectedPath) == 0)){
        printf("This process should be named producer before executing.\n");
        currentDateTime = getDateTime();
        snprintf(event, sizeof event, "%s, [202], This process should be named producer before executing.\n", currentDateTime);
        logEvent(event,"error.log");
        exit(1);
    }
    // Check if minimum required parameters entered.
    if (argc < 2) help();
    else if(strcmp(argv[1], "--help") == 0) help();
    else if(strcmp(argv[1], "--version") == 0) printVersion();
    else if(argc< MIN_REQUIRED) help();
    // Array for input values to be stored in
    static int inputs[3];

    // Let's create some input switches
    const char *switches[3];
    switches[0] = "--max-jobs";
    switches[1] = "--jobs-per-cycle";
    switches[2] = "--log-level";

    const char *shortSwitches[3];
    shortSwitches[0] = "-m";
    shortSwitches[1] = "-j";
    shortSwitches[2] = "-l";

  char *endptr; 

  for (i = 1;i < argc;i+=2){

    for (j = 0;j < 3;j++){
      if(strcmp(argv[i], switches[j]) == 0) if (!isInt(argv[i+1], &inputs[j])) help(); // Match the input switch to it's variable, if invalid call help and exit.
      if(strcmp(argv[i], shortSwitches[j]) == 0) if (!isInt(argv[i+1], &inputs[j])) help();
      if (strcmp(argv[i], "--verbose") == 0) verbose=true; // Check for verbose output request.
      if (strcmp(argv[i], "-v") == 0) verbose=true; 
    }
  }
  bool exitProcess;
  if (inputs[1]>100){
       printf("Warning! You have told Producer to create over 100 jobs per second, are sure you want to do this?\n\n");
       exitProcess = promptUser();
       if (exitProcess) exit(1);
  }
  if (inputs[0] < inputs[1] ){
       printf("Warning! Max jobs value must be equal to or higher than jobs per cycle. See the help page below.\n\n");
       currentDateTime = getDateTime();
       snprintf(event, sizeof event, "%s, [203], Warning! Max jobs value must be equal to or higher than jobs per cycle.\n", currentDateTime);
       logEvent(event,"error.log");
       help();
  }
  else if (inputs[2]< 0 || inputs[2] > 2){
       printf("Warning! You have inputted an incorrect value for log level. See the help page below.\n\n");
       currentDateTime = getDateTime();
       snprintf(event, sizeof event, "%s, [204], Warning! You have inputted an incorrect value for log level.\n", currentDateTime);
       logEvent(event,"error.log");
       help();
  }
  else if (inputs[0] < 1 || inputs[1] < 1){
       printf("Warning! Max jobs and jobs per cycle must be over zero. See the help page below.\n\n");
       currentDateTime = getDateTime();
       snprintf(event, sizeof event, "%s, [205], Warning! Max jobs and jobs per cycle must be over zero.\n", currentDateTime);
       logEvent(event,"error.log");
       help();
  }
  if (verbose==true){
      printf("Passed sanity checks.\n");
      printf("Verbose output:\ton\n");
      printf("Max jobs:\t%d\nJobs per cycle:\t%d\nLog level:\t%d\n",inputs[0],inputs[1],inputs[2]);
    }
  // Create meaningful names for our inputs.
  int maxJobs, jobsPerCycle, logLevel;
  maxJobs = inputs[0];
  jobsPerCycle = inputs[1];
  logLevel = inputs[2];
  // If no access log exists then create one with useful headers.
  if (logLevel > 0 && !file_exist ("access.log")) logEvent("Date, Event Key, Event\n\n","access.log");
  // Declare PID variables to store fork pointers
	pid_t pid, pid2, sendSizeChild;
	int randomPriority,loopCount,createdJobs;
    exitProcess = false;
    int sharedSizeSHMID;
    sharedSizeSHMID = sendSharedSize(maxJobs);
	struct vector *array;
	//shmctl(shmid, IPC_RMID, NULL);
	int sharedDataSHMID;
	array = setSharedMem(maxJobs,&sharedDataSHMID,logLevel);
    array[0].cycleCount = 0; // Zero initialize and store in first element of the array so that it can be shared.
    i = 0;
    // Zero initialize the array via priority key.
    for (k;k < maxJobs;k++){
            array[i].priority = 0;
        }
     // Start job creation and destruction cycle.
	while(!exitProcess){
                loopCount = i+jobsPerCycle;
		if (loopCount>maxJobs) loopCount = maxJobs;
		createdJobs = loopCount-i;
                for (i;i < loopCount;i++){
                    pid_t tempPID;
				    tempPID = fork();
				    if (tempPID!=0){
                        randomPriority = (rand() % 10)+1;
					    array[i].pid = tempPID;
					    array[i].priority = randomPriority;
                        // Open file for logging. Key: [101] = created job, [102] = destroyed job.
                        currentDateTime = getDateTime();
                        if (verbose==true) printf("%s: Job created with PID %d and priority %d\n", currentDateTime,tempPID,randomPriority);
                        else printf("Job created with PID %d and priority %d\n",tempPID,randomPriority);
                        snprintf(event, sizeof event, "%s, [101], Job created with PID %d and priority %d\n", currentDateTime,tempPID,randomPriority);
                        if (logLevel > 0) logEvent(event,"access.log");
                        sleep(1);
					    }
                    else while (pause());
				    }
                array[0].jobsAlive = returnAliveJobs(array,maxJobs);
                sleep(1); // Give consumer time to sync the cycle.
                array[0].cycleCount++;
                sleep(1); // Wait long enough for consumer to delete jobs.
                printf("Cycle %d results: In this cycle %d jobs were killed and %d jobs were created. There are currently %d alive jobs.\n",array[0].cycleCount, array[0].killPerCycle, createdJobs, returnAliveJobs(array,maxJobs));
            if (maxJobs<=(array[0].cycleCount*jobsPerCycle)) exitProcess = true;  
            else if(array[0].cycleCount%5==0) exitProcess = promptUser();
        // Since forks are still running let's kill them on exit=true
        if (exitProcess){
             system("./cleansharedmem >/dev/null 2>&1;killall -9 consumer >/dev/null 2>&1;killall -9 producer >/dev/null 2>&1");
            }
		}
}
