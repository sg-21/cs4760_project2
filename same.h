#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

extern int opterr;

//semiphore union
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;

#if defined(__linux__)
    struct seminfo *__buf;
#endif

};

//shared memory 
struct SharedItem {
    bool ready;       //ready to process           
    pid_t pid;        //in process     
    bool finished;    //complete process         
    int nodeDepth;    //depth node processing         
    int value;        //actual value             
    state nodeState;  //critical section flag       
};

key_t key = 0;        //shared key
int shm_id; 
char* shm_addr;

const char* host = "./master";
const char* child = "./bin_adder";

//time formatting
string GetStringFromInt(const int nVal) {
    int length = snprintf( NULL, 0, "%d", nVal);
    char* sdep = (char*)malloc( length + 1 );

    snprintf( sdep, length + 1, "%d", nVal);
    string strFinalVal = sdep;

    free(sdep);
    return strFinalVal;
}

//return string from int
string GetTimeFormatted(const char* prePendString) {
    time_t raw_t;
    struct tm * timeinfo;
    char buffer[10];

    time (&raw_t);
    timeinfo = localtime (&raw_t);

    strftime (buffer, 80, "%T", timeinfo);

    string strReturn = prePendString;
    strReturn.append(buffer);
    return strReturn;
}

