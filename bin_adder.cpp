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
#include <vector>
#include <math.h>

using namespace std;

const int maxChildProcess = 19;
const int maxTimeSeconds = 100;

struct SharedMemory {
        bool ready;
        pid_t pid;
        int depth;
        int value;
};

key_t key;
int shmId;
struct SharedMemory* shmem;

void attachShmem();
void signalHandler();
void timer();

int main(int argc, char * argv[]) {

	key = ftok("makefile", 'p');
	
	pid_t childPid = getpid();

	int index = 16;

	attachShmem();
	
	int firstIndex = atoi(argv[0]);
	int depth = atoi(argv[1]);

	int secondIndex = pow(2,depth) + firstIndex;

	shmem[firstIndex].pid = shmem[secondIndex].pid = childPid;

	//adds the first two values and stores it in the first index
	//shmem[firstIndex].value = shmem[firstIndex].value + shmem[secondIndex].value;

	return 0;

}

void attachShmem() {

	if((shmId = shmget(key, sizeof(struct SharedMemory), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
                perror("bin_adder.cpp: error: failed to allocate shared memory");
                exit(EXIT_FAILURE);
        }
        else {
                shmem = (struct SharedMemory*)shmat(shmId, NULL, 0);
        }

}

void signalHandler() {}

void timer() {}
