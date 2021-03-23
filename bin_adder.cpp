#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fstream>
#include <signal.h>
#include <math.h>
#include <algorithm>
#include <stdarg.h>

#include "bin_adder.h"
#include "same.h"

using namespace std;

volatile sig_atomic_t sigQuitFlag = 0;

void sigQuitHandler(int sig) {
  sigQuitFlag = 1; 
}

const char* outputFile = "adder_log";

struct SharedItem* sharedMem;

/*prototypes*/
void attachSharedMemory();
void processAlgorithm(int, int, int, int);
char* getTimeFormatted();

int main(int argc, char* argv[]) {
    signal(SIGINT, sigQuitHandler);
  
    key = ftok("makefile", 'p');

    /*stored PID in shared memory*/
    pid_t childPid = getpid();

    attachSharedMemory();

    /*adding values*/  
    int firstIndex = atoi(argv[0]);
    int depth = atoi(argv[1]);
    int secondIndex = pow(2, depth) + firstIndex;	
    int pidPosition = atoi(argv[2]);

    /*indexes set to childPID for adding, then stores first which is passed*/
    sharedMem[firstIndex].pid = sharedMem[secondIndex].pid = childPid;
    sharedMem[firstIndex].value = sharedMem[firstIndex].value + sharedMem[secondIndex].value;	

    /*attempted Bakery Algorithm*/
    processAlgorithm(pidPosition, firstIndex, secondIndex, depth);	

    ofstream ofoutputFile (outputFile, ios::app);

     if (ofoutputFile.is_open()) {
         ofoutputFile << getTimeFormatted() << "\t"
                    << childPid   << "\t"
                    << firstIndex << "\t"
                    << depth << endl;
         ofoutputFile.close();
      }
	return 0;

}

/*attaching shared memory*/
void attachSharedMemory() {

	if((shm_id = shmget(key, sizeof(struct SharedItem), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
                perror("bin_adder.cpp: ERROR: failed to allocate shared memory");
                exit(EXIT_FAILURE);
        }
        else {
                sharedMem = (struct SharedItem*)shmat(shm_id, NULL, 0);
        }

}

/*my attempted bakery algorithm implementation - using online sources*/
void processAlgorithm(int v1, int pidIndex, int index, int numDepth) {
	do {
		sharedMem->choosing[v1] = true;
		sharedMem->number[v1] = 1 + (long) std::max_element(sharedMem->number, sharedMem->number + (v1-1));
		sharedMem->choosing[v1] = false;

    for(int j = 0; j < maxChild; j++) {

while(sharedMem->choosing[j]);
			while(sharedMem->number[j] && (sharedMem->number[j] < sharedMem->number[v1] || (sharedMem->number[v1] == sharedMem->number[v1] && j < v1)));
		}
		
		/* Critical Section */
		wait(NULL);
		FILE * fp = fopen("adder_log", "a");

    fprintf(fp, "%s \t%d \t%d \t%d \n", getTimeFormatted(), sharedMem[pidIndex].pid, pidIndex, numDepth);
		fprintf(fp, "%s\t Entering the critical section\n", getTimeFormatted());	



fflush(stdout);	
		fprintf(fp, "%s\t Exiting Critical section\n", getTimeFormatted());
		fclose(fp);

		sharedMem->number[v1] = 0;

		break;

	}while(1);

} 
