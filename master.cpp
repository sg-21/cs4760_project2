#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "master.h"
#include "same.h"

const int MAX_PROCESSES = 19;
static int myCount = 0;

/*constants*/
const int maxChild = 20;
const int maxSecond = 100;

static void usage(std::string);
using namespace std;

/*variables*/
vector<int> myArray;
int* num;
struct SharedItem* node;

/*handling*/
volatile sig_atomic_t sigIntFlag = 0;

void sigintHandler(int sig){
  sigIntFlag = 1; 
}

/*process data from input file*/
int processMaster(int numChild, int seconds, string dataFile) {
    signal(SIGINT, sigintHandler);
    bool dead = false;

    /*time start*/
    time_t start;

    FILE *fileName;
    char *line = NULL;

    size_t len = 0;
    ssize_t read;

    fileName = fopen(dataFile.c_str(), "r");
    if (fileName == NULL) {
        errno = ENOENT;
        perror(dataFile.c_str());

        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fileName)) != -1) {
        int i = atoi(line);

        /*line placed in vector*/
        myArray.push_back(i);  
    }

    fclose(fileName);
    free(line);

    /*time start*/
    start = time(NULL);

    /*checks if power of 2*/
    int index = myArray.size();
    int level = 0;

    while (index >>= 1) ++level;

    if(pow(2, level) < myArray.size()) {
        level++;

        do {
            /*padding array with zeros*/
            myArray.push_back(0);
        }

        while(pow(2, level) > myArray.size());
    }

    /*ready to process file*/
    int itemCount = myArray.size();

    /*read/write*/
    if ((key = ftok(Host, 100)) == -1) {
        perror("ftok");

        exit(EXIT_FAILURE);
    }

    else {
        /*allocate shared memory with size of struct array*/
        int memSize = sizeof(SharedItem) * itemCount;
        shm_id = shmget(key, memSize, IPC_CREAT | IPC_EXCL | 0660);

        if (shm_id == -1) {
            perror("shmget: ");
            exit(EXIT_FAILURE);
        }
    }

    /*attach shared memory to address space*/
    shm_addr = (char*)shmat(shm_id, NULL, 0);

    if (!shm_addr) { 
        perror("shmat: ");
        exit(EXIT_FAILURE);
    }

    num = (int*) shm_addr;
    *num = 0;
    node = (struct SharedItem*) (shm_addr+sizeof(int));

    for(int i=0; i < itemCount; i++) {
        node[i].ready = true; 
        node[i].pid = 0;
        node[i].finished = false;
        node[i].nodeDepth = -1;
        node[i].value = myArray[i];
        node[i].nodeState = idle;
    }

    /*start process with bin_adder xx yy*/
    bool done = false;
    pid_t waitPID;
    int wstatus;

    /*print node*/
    for(int j=0; j < itemCount; j++)
        cout << "\t" << node[j].value;
        cout << endl;

    int nDepth = level;

    /*loop until calculation complete*/
    while(!done) {

        /*search for "ready" node by depth*/
        if(!sigIntFlag && !((time(NULL)-start) > seconds) && myCount < numChild && myCount < MAX_PROCESSES) {

            for(int i=0;i<nDepth;i++) {

                for(int j=0; j < itemCount && myCount < numChild && myCount < MAX_PROCESSES; j += pow(2, i+1)) {
                    int ncheck1 = j;
                    int ncheck2 = pow(2, i) + j;

                    if(node[ncheck1].nodeDepth < i && node[ncheck1].ready && node[ncheck2].ready && node[ncheck1].nodeDepth == node[ncheck2].nodeDepth) {
                        /*processing*/ 
                        node[ncheck1].ready = node[ncheck2].ready = false;
                        /*depth of last process*/
                        node[ncheck1].nodeDepth = node[ncheck2].nodeDepth = i;

                        myCount++;

                        /*fork and store PID*/
                        int pid = forkProcess(ncheck1, i);
                        node[ncheck1].pid = node[ncheck2].pid = pid;
                    }
                }
            }
        }

        /*search for returning PID*/
        do {
            waitPID = waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED);

            /*terminate if Ctrl-c*/
            if((sigIntFlag || (time(NULL)-start) > seconds) && !dead) {
                dead = true;

                /*sends signal for every child to terminate*/
                for(int i=0;i<itemCount;i++) {
                    if(node[i].ready == false)
                        kill(node[i].pid, SIGQUIT);
                }

                cout << endl;

                if(sigIntFlag) {
                    errno = EINTR;
                    perror("Killing processes -> Ctrl-c signal");
                }

                else {
                    errno = ETIMEDOUT;
                    perror("Killing processes - Timeout");
                }
            }

            /*no PIDs*/
            if (waitPID == -1) {

                if(!dead) {
                    int length = snprintf( NULL, 0, "%d", node[0].value);
                    char* sdep = (char*)malloc( length + 1 );

                    snprintf( sdep, length + 1, "%d", node[0].value);
                    string strFinalVal = " Addition Process Complete: ";

                    strFinalVal.append(sdep);
                    free(sdep);

                    /*success error code*/
                    errno = 0;

                    cout << endl;
                    perror(strFinalVal.c_str());
                }
                done = true;   
                break;
            }

            if (WIFEXITED(wstatus) && waitPID > 0) {
                myCount--;

                /*show successful child process*/
                for(int j=0; j < itemCount; j++)
                    cout << "\t" << node[j].value << endl;

                int length = snprintf( NULL, 0, "%d", waitPID);
                char* sdep = (char*)malloc( length + 1 );

                snprintf( sdep, length + 1, "%d", waitPID );
                string strPID = sdep;

                free(sdep);

                /*add time component*/ 
                strPID.append(" Exited: ");

                string strFormattedResult = GetTimeFormatted(strPID.c_str());
                perror(strFormattedResult.c_str());

                if(node[0].pid == waitPID && node[0].nodeDepth==nDepth) {
                    done = true;
                    break;
                }

                else {

                    for(int i=0;i<itemCount;i++) {

                        if(node[i].pid == waitPID) {
                            node[i].ready = true;
                            break;
                        }
                    }
                }
                
            } else if (WIFSIGNALED(wstatus) && waitPID > 0) {
                cout << waitPID << " Killed by Signal " << WTERMSIG(wstatus) << endl;

            } else if (WIFSTOPPED(wstatus) && waitPID > 0) {
                cout << waitPID << " Stopped by Signal " << WTERMSIG(wstatus) << endl;

            } else if (WIFCONTINUED(wstatus) && waitPID > 0) {
                continue;
            }

        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
    }

    /*detach shared memory*/
    cout << endl;
    perror("Detatch Shared Memory");

    if (shmdt(shm_addr) == -1) {
        perror("main: shmdt: ");
    }

    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("main: shmctl: ");
    }

    perror("Shared Memory Detatched");
    cout << endl;

    return EXIT_SUCCESS;
}

int forkProcess(int start, int depth) {
        int pid = fork();

        /*signal for child process to exit*/
        if(pid < 0) {
            perror("Could not fork process");
            return EXIT_FAILURE;
        }
        
        if(pid == 0) {

            /*string version of nItemStart*/
            int length = snprintf( NULL, 0, "%d", start);
            char* sStart = (char*)malloc( length + 1 );
            snprintf( sStart, length + 1, "%d", start );

            string strItemStart = sStart;
            free(sStart);

            /*string version of nDepth*/
            length = snprintf( NULL, 0, "%d", depth);
            char* sdep = (char*)malloc( length + 1 );
            snprintf( sdep, length + 1, "%d", depth );

            string strDepth = sdep;
            free(sdep);

            execl(Child, strItemStart.c_str(), strDepth.c_str(), NULL);

            fflush(stdout);
            exit(EXIT_SUCCESS);    
        }
        
        else
            return pid; 


int main(int argc, char* argv[]) {
    int option;

    /*default*/
    int numSecond = 100; 
    int numChild = 20; 

    while ((option = getopt(argc, argv, "hs:t:")) != -1) {
        switch (option) {
            case 'h':
                usage(argv[0]);
                return EXIT_SUCCESS;
            case 's':
                numChild = atoi(optarg);
                break;
            case 't':
                numSecond = atoi(optarg);
                break;
            case '?': 
                if (isprint (optopt)) {
                    errno = EINVAL;
                    perror("Unknown Option");
                }
                else {
                    errno = EINVAL;
                    perror("Option Character Unknown.");
                }
                return EXIT_FAILURE;
            default:
                perror ("master: ERROR: Invalid Argument Found.");
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    numChild = min(numChild, maxChild);
    numSecond = min(numSecond, maxSecond);

    /*checking for datafile passed*/
    int index = optind;

    if(index < argc) {
        string File = argv[index];

        cout << "Master Process Start: " << endl
            << "\t" << numChild << " Max Processes " << endl
            << "\t" << numSecond  << " Max Seconds " << endl 
            << endl;

        return processMaster(numChild, numSecond, File);
    }

    perror ("ERROR: Provide datafile to process");
    usage(argv[0]);
    return EXIT_FAILURE;
}

/*handle errors through usage*/
static void usage(std::string name) {
    std::cerr << std::endl
              << name << " - master" << std::endl
              << std::endl

              << "Usage:\t" << name << " [-h]" << std::endl
              << "\t" << name << " [-h] [-s i] [-t time] datafile" << std::endl

              << "Options:" << std::endl
              << "  -h        Help menu to describe how the project is run." << std::endl
              << "  -s x      The number of children allowed to exist in the system at the same time. (Default 20)" << std::endl
              << "  -t time   The time in seconds after which the process will terminate, even if it has not finished. (Default 100)"
              << "  datafile  Input file containing one integer on each line."
              << std::endl << std::endl;
}


