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


const int maxChildProcess = 20;
const int maxTimeSeconds = 100;

struct SharedMemory {
        bool ready;
        bool done;
        pid_t pgid;
        int depth;
        int value;
	int vecSize;
};

key_t key;
int shmId;
struct SharedMemory* shmem;

void attachShmem();

int main(int argc, char * argv[]) {

	key = ftok("makefile", 'p');

	attachShmem();

	cout << "BIN_ADDER:\t\t";

	for(int i = 0; i < shmem->vecSize; i++)
		cout << "" << shmem[i].value << " ";
	
	cout << endl;


	return 0;

}

void attachShmem() {

	if((shmId = shmget(key, sizeof(struct SharedMemory), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
                perror("master.cpp: error: failed to allocate shared memory");
                exit(EXIT_FAILURE);
        }
        else {
                shmem = (struct SharedMemory*)shmat(shmId, NULL, 0);
        }
}
