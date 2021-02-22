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

void attachShmem();
void releaseShmem();
void deleteShmem();
void spawnChild(int);
void storeShmem(vector<int>, int);

void usage();

key_t key;
int shmId;
struct SharedMemory* shmem;

int main(int argc, char * argv[]) {

	int opt;
	
	int userChildren = 20;
	int userTime = 100;

	char * dataFile;

	while((opt = getopt(argc, argv, "hs:t:")) != -1) {
		switch(opt) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			case 's':
				if(!isdigit(*optarg) || (userChildren = atoi(optarg)) < 0 || (userChildren = atoi(optarg)) > 20) {
					perror("master.cpp: error: invalid number of processes");
					usage();
					exit(EXIT_FAILURE);
				}
				break;
			case 't':
				if(!isdigit(*optarg) || (userTime = atoi(optarg)) < 0 || (userTime = atoi(optarg)) > 100) {
					perror("master.cpp: error: invalid process timeout");
					usage();
					exit(EXIT_FAILURE);
				}
				break;
			default:
				perror("master.cpp: error: pleasue use -h for more info\n");
				usage();
				exit(EXIT_FAILURE);
		}
	}
	
	//key genereated for shared memory
	key = ftok("makefile", 'p');

	//allocates and attaches shared memory
	attachShmem();

	if(argv[optind] == NULL) {
		perror("master.cpp: error: no input file");
		exit(EXIT_FAILURE);
	}
	
	dataFile = argv[optind];

	FILE *fp = fopen(dataFile, "r");
	int errVal;
	vector<int> numberHolder;
	char *readLine = NULL;
	size_t len = 0;
	ssize_t read;

	if(fp == NULL) {
		perror("master.cpp: error: failed to open file");
		exit(EXIT_FAILURE);
	}
	
	while(read = getline(&readLine, &len, fp) != -1) {
		int index = atoi(readLine);
		numberHolder.push_back(index);
	}
	fclose(fp);
	free(readLine);

	int index = numberHolder.size();
        int depth = 0;

        while (index >>= 1) {
                depth++;
        }

        if(pow(2,depth) < numberHolder.size()) {
		depth++;	
                do {
			numberHolder.push_back(0);
             	}
		while(numberHolder.size() < pow(2,depth));
        }

	cout << depth << " \t\t\t";

	storeShmem(numberHolder, numberHolder.size());

	shmem->vecSize = numberHolder.size();

	execl("./bin_adder", (char *)NULL);

	releaseShmem();
	deleteShmem();

	return 0;

}

void spawnChild(int p) {
	
	pid_t pid = fork();

	if(pid == -1) {
		perror("master.cpp: error: failed to create child process");
		exit(EXIT_FAILURE);
	}
	
	if(pid == 0) {
		if(p == 0) {
			shmem->pgid = getpid();
		}

		setpgid(0, shmem->pgid);

	}
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

void releaseShmem() {

	if(shmem != NULL) {
		if(shmdt(shmem)) {
			perror("master.cpp: error: failed to release shared memory");
			exit(EXIT_FAILURE);
		}
	}
}

void deleteShmem() {
	
	if(shmId > 0) {
		if(shmctl(shmId, IPC_RMID, NULL) < 0) {
			perror("master.cpp: error: failed to delete shared memory");
			exit(EXIT_FAILURE);
		}
	}
}

void storeShmem(vector<int> vect1, int size1) {

	for(int i = 0; i < size1; i++) {
                shmem[i].value = vect1[i];
                shmem[i].pgid = 0;
                shmem[i].done = false;
                shmem[i].ready = true;
                shmem[i].depth = -1;
        }

        for(int i = 0; i < size1; i++) {
                cout << shmem[i].value << " ";
        }
	cout << endl;

}

void usage() {
	printf("======================================================================\n");
	printf("\t\t\t\tUSAGE\n");
	printf("======================================================================\n");
	printf("master -h [-s x] [-t time]\n");
	printf("-h		:	Describes how the project should be run and then terminates.\n");
	printf("-s x		:	Indicate the number of children allowed to exist in the system at the same time (Default 20)\n");
	printf("-t time 	:	Time in seconds after which the process will terminate, even if it has not finished (Default 100)\n");
	printf("datafile	:	Input file containing one integer on each line\n");

}

