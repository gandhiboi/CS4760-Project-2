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
        pid_t pid;
        int depth;
        int value;
};

void attachShmem();
void releaseShmem();
void deleteShmem();
void spawnChild(int, int, int);
void storeShmem(vector<int>, int);
void signalHandler();
void timer();

void usage();

static int pCounter = 0;
const int MAX_CHILD = 19;

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
	readLine = NULL;

	int index = numberHolder.size();
        int depth = 0;

	depth = log(index) / log(2);	

        if(pow(2,depth) < numberHolder.size()) {
		depth++;	
                do {
			numberHolder.push_back(0);
             	}
		while(numberHolder.size() < pow(2,depth));
        }

	storeShmem(numberHolder, numberHolder.size());

	bool terminateFlag = false;

	pid_t parentId = getpid();
	
	cout << "parentId: " << parentId << endl;

	while(!terminateFlag) {
		for(int i = 0; i < depth; i++) {
			for(int j = 0; j<numberHolder.size() && pCounter < maxChildProcess && pCounter < 20; j+=pow(2,i+1)) {
				int forkIndexer = j;
				int depthIndexer = pow(2,i) + j;
				if(shmem[forkIndexer].ready && shmem[depthIndexer].ready && shmem[forkIndexer].depth == shmem[depthIndexer].depth) {
					shmem[forkIndexer].ready = shmem[depthIndexer].ready = false;
					shmem[forkIndexer].depth = shmem[depthIndexer].depth = i;
					pCounter++;
					spawnChild(forkIndexer, i, depthIndexer);
				}
			}
		}
		terminateFlag = true;
	}

	releaseShmem();
	deleteShmem();

	return EXIT_SUCCESS;

}

void spawnChild(int forkIndex, int forkDepth, int number2) {
	
	pid_t pid = fork();

	if(pid == -1) {
		perror("master.cpp: error: failed to create child process");
		exit(EXIT_FAILURE);
	}
	
	if(pid == 0) {	

		int length = snprintf(NULL, 0, "%d", forkIndex);
		char* xx1 = (char*)malloc(length + 1);
		snprintf(xx1, length + 1, "%d", forkIndex);

		length = snprintf(NULL, 0, "%d", forkDepth);
		char* yy1 = (char*)malloc(length + 1);
		snprintf(yy1, length + 1, "%d", forkDepth);

		execl("./bin_adder", xx1, yy1, (char *)NULL);
		free(xx1);
		free(yy1);

		xx1 = NULL;
		yy1 = NULL;

		exit(EXIT_SUCCESS);

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
                shmem[i].pid = 0;
                shmem[i].ready = true;
                shmem[i].depth = -1;
        }
}

void signalHandler() {	

}

void timer(){}

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

