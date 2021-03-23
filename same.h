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

/*semiphore union*/
union semun {
    int                 val;
    struct semid_ds *   buf;
    unsigned short *    array;

#if defined(__linux__)
    struct seminfo *    __buf;
#endif

};

const int maxChild = 20;
const int maxSecond = 100;

/*shared memory*/
struct SharedItem {
    bool choosing[maxChild];
    int number[maxChild];
    bool ready;            
    pid_t pid;              
    bool finished;          
    int nodeDepth;          
    int value;                      
};

/*shared key*/
key_t key = 0;  
int shm_id; 
char* shm_addr;

const char* Host = "./master";
const char* Child = "./bin_adder";

/*return string from int*/
string GetStringFromInt(const int nVal) {
    int length = snprintf( NULL, 0, "%d", nVal);
    char* sDep = (char*)malloc( length + 1 );
    snprintf( sDep, length + 1, "%d", nVal);
    string strFinalVal = sDep;
    free(sDep);
    return strFinalVal;
}

/*time formatting*/
char* getTimeFormatted() {

	int timeLength = 50;
        char* formatTime = new char[timeLength];
        time_t now = time(NULL);

        strftime(formatTime, timeLength, "%H:%M:%S", localtime(&now));
        return formatTime;

}

