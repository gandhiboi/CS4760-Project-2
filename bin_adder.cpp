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


using namespace std;

const int maxChildProcess = 20;
const int maxTimeSeconds = 100;

struct SharedMemory {
        bool choosing[maxChildProcess];
	int number[maxChildProcess];
        pid_t pid;
        int depth;
	int vectSize;
        int value;
};

//sames as master; used for shared mem
key_t key;
int shmId;
struct SharedMemory* shmem;

//func prototypes
void attachShmem();
void signalHandler(int);
void process(int, int, int, int);
char* getTime();

int main(int argc, char * argv[]) {

	//handles timer and ctrl+c
	signal(SIGTERM, signalHandler);
	signal(SIGALRM, signalHandler);

	//same key as used in master
	key = ftok("makefile", 'p');
	
	//used to store pid in shared memory
	pid_t childPid = getpid();

	//self explanatory
	attachShmem();

	//indices used for adding the values together
	int firstIndex = atoi(argv[0]);						//xx = 0,2,4,6,...
	int depth = atoi(argv[1]);						//yy = 0,1,2,3,...
	int secondIndex = pow(2, depth) + firstIndex;				//index for accessing 1,3,5,6,...
	int pidPosition = atoi(argv[2]);					//zz = pCounter

	//debugging purposes
	//cout << "firstIndex: " << firstIndex << endl;					//0,2,4,6
	//cout << "secondIndex: " << secondIndex << endl;				//1,3,5,7

	//sets the first and second index to the same childPid since theyre performing the additions
	shmem[firstIndex].pid = shmem[secondIndex].pid = childPid;

	//adds the corresponding indices and stores it in the first index that is passed
	shmem[firstIndex].value = shmem[firstIndex].value + shmem[secondIndex].value;		//0+1,2+3,4+5,6+7

	//bakery algorithm
	process(pidPosition, firstIndex, secondIndex, depth);	

	return 0;

}

//attaches to shared memory set up in master.cpp
void attachShmem() {

	if((shmId = shmget(key, sizeof(struct SharedMemory), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
                perror("bin_adder.cpp: error: failed to allocate shared memory");
                exit(EXIT_FAILURE);
        }
        else {
                shmem = (struct SharedMemory*)shmat(shmId, NULL, 0);
        }

}

//implementation of bakery algorithm; stores time, pid, index, and depth to adder_log
void process(int var1, int pidIndex, int index, int numberDepth) {
	do {
		shmem->choosing[var1] = true;
		shmem->number[var1] = 1 + (long) std::max_element(shmem->number, shmem->number + (var1-1));
		shmem->choosing[var1] = false;
		for(int j = 0; j < maxChildProcess; j++) {
			//cout << "j: " << j << endl;
			while(shmem->choosing[j]);
			while(shmem->number[j] && (shmem->number[j] < shmem->number[var1] || (shmem->number[var1] == shmem->number[var1] && j < var1)));
		}
		
		/* ~~~~~~ critical section ~~~~~~~ */
		wait(NULL);
		FILE * fp = fopen("adder_log", "a");
		fprintf(fp, "%s \t%d \t%d \t%d \n", getTime(), shmem[pidIndex].pid, pidIndex, numberDepth);
		fclose(fp);
		//cout << "bin_adder addition: " << shmem[indexer].value << endl;	
		fflush(stdout);	
		/* ~~~~~~ exit critical section ~~~~~~ */

		shmem->number[var1] = 0;

		break;

	}while(1);

}

//used to handle ctrl+c and timer
void signalHandler(int s) {

	if(s == SIGTERM) {
		printf("bin_adder: signalHandler is terminating processes");
	}
	else if(s == SIGALRM) {
		printf("bind_adder: signalHandler has reached timeout value");
	}

     	for(int i = 0; i < shmem->vectSize; i++) {
     	  	kill(shmem[i].pid, SIGQUIT);
     	}
	
	exit(EXIT_FAILURE);

}

//used to get current system time
char* getTime() {

        int timeLength = 50;
        char* formatTime = new char[timeLength];
        time_t now = time(NULL);

        strftime(formatTime, timeLength, "%H:%M:%S", localtime(&now));
        return formatTime;

}

