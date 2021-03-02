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
#include <stdarg.h>

using namespace std;

//hard cap for child processes and time
const int maxChildProcess = 20;
const int maxTimeSeconds = 100;

//shared memory structure 
struct SharedMemory {
        bool choosing[maxChildProcess];
	int number[maxChildProcess];
        pid_t pid;
	int vectSize;
        int depth;
        int value;
};

//func prototypes
void attachShmem();
void releaseShmem();
void deleteShmem();
void spawnChild(int, int, int);
void storeShmem(vector<int>, int);
void signalHandler(int);
void setTimer(int);
char* getTime();

//help menu
void usage();

int pCounter = 0;					//process counter - used for bakery algorithm and preventing number of processes exceeding hardcap
key_t key;						//key for shared memory	
int shmId;						//used for shared memory id
struct SharedMemory* shmem;				//object to access shared memory structure

int main(int argc, char* argv[]) {

	//signal handler for ctrl+c and timer
	signal(SIGINT, signalHandler);
	
	int opt;

	//default values for number of children and timer
	int userChildren = 20;
	int userTime = 100;

	char* dataFile;

	while((opt = getopt(argc, argv, "hs:t:")) != -1) {
		switch(opt) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			case 's':
				//checks to see if the value of number of children is valid between 0-20; if so store else exit
				if(!isdigit(*optarg) || (userChildren = atoi(optarg)) < 0 || (userChildren = atoi(optarg)) > 20) {
					perror("master.cpp: error: invalid number of processes");
					usage();
					exit(EXIT_FAILURE);
				}
				break;
			case 't':
				//checks to see if the timeout value is valid between 0-100; if so store else exit
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

	//error checking to see if input file is provided
	if(argv[optind] == NULL) {
		perror("master.cpp: error: no input file");
		exit(EXIT_FAILURE);
	}
	else {
		dataFile = argv[optind];
	}

	//preparation to read integers into vector
	FILE *fp = fopen(dataFile, "r");
	vector<int> numberHolder;
	char *readLine = NULL;
	size_t len = 0;
	ssize_t read;

	//error checking to see if file can be opened
	if(fp == NULL) {
		perror("master.cpp: error: failed to open file");
		exit(EXIT_FAILURE);
	}
	
	//reads file into vector
	while(read = getline(&readLine, &len, fp) != -1) {
		int index = atoi(readLine);
		numberHolder.push_back(index);
	}
	fclose(fp);
	free(readLine);
	readLine = NULL;

	//used in nested loops and padding of the vector to a power of 2
	int index = numberHolder.size();
        int depth = 0;

	//calculates the depth based on a power two 
	depth = log(index) / log(2);	

	//determines whether or not to pad the vector; if 2^depth is less than the number of ints in vector
        if(pow(2,depth) < numberHolder.size()) {
		depth++;	
                do {
			numberHolder.push_back(0);
             	}
		while(numberHolder.size() < pow(2,depth));
        }

	//stores the vector into shared memory and initializes variables to defaults
	storeShmem(numberHolder, numberHolder.size());

	shmem->depth = depth;

	//used for indexing through the values in shared memory to do the addition
	int power = 1;
	int indexer = 1;

	shmem->vectSize = numberHolder.size();
	
	//used to display all integers in the largest depth
	cout << "Depth: " << depth << "\t" << "values:\t";
	for(int i = 0; i < numberHolder.size(); i++) {
		cout << shmem[i].value << " ";
	}
	cout << endl;
	
	//sets up timer for the program with user selected time or default value
	setTimer(userTime);

	//outer loop is responsible for the depth
	//could have used power variable but i didnt wanna break my project again
	for(int i = 0; i < depth; i++) {
		indexer = pow(2, power);								//increment = 2,4,8,... used for indices that will be sent to bin_adder to access values in shmem
		
		//loops with the indexer to send index to bin_adder until reaches size of vector
		for(int x = 0; x < numberHolder.size(); x += indexer) {
			sleep(1);
			if(pCounter < userChildren) {
				pCounter++;
				spawnChild(x, i, pCounter);
				FILE * fp = fopen("output.log", "a");
                		if(fp == NULL) {
                        		perror("master.cpp: error: failed to open output.log");
                		}
                		fprintf(fp, "%s \t Process %d finished\n", getTime(), pCounter);
                		fclose(fp);
			}
			else if(pCounter == userChildren) {
				pCounter--;
			}
		}
		power++;
	}

	sleep(1);

	cout << "Total sum of pairs: " << shmem[0].value << endl;

	//releases and deletes shared memory
	releaseShmem();

	FILE * fp1 = fopen("output.log", "a");
        if(fp1 == NULL) {
        	perror("master.cpp: error: failed to open output.log");
        }
        fprintf(fp1, "%s \t Calculations completed successfully\n", getTime());
        fclose(fp1);

	return EXIT_SUCCESS;

}

//responsible for forking and execl
void spawnChild(int forkIndex, int forkDepth, int counter) {
	
	pid_t pid = fork();

	if(pid == -1) {
		perror("master.cpp: error: failed to create child process");
		exit(EXIT_FAILURE);
	}
	
	if(pid == 0) {			

		//converts the indices to c string to pass into execl
		int length = snprintf(NULL, 0, "%d", forkIndex);
		char* xx = (char*)malloc(length + 1);
		snprintf(xx, length + 1, "%d", forkIndex);

		//converts the depth to c string to pass into execl
		length = snprintf(NULL, 0, "%d", forkDepth);
		char* yy = (char*)malloc(length + 1);
		snprintf(yy, length + 1, "%d", forkDepth);

		//convers the process id to c string to pass into execl
		length = snprintf(NULL, 0, "%d", counter);
		char* zz = (char*)malloc(length + 1);
		snprintf(zz, length + 1, "%d", zz);

		//logs that process is starting into output.log
		FILE * fp = fopen("output.log", "a");
		if(fp == NULL) {
			perror("master.cpp: error: failed to open output.log");
		}
		fprintf(fp, "%s \t Process %d started\n", getTime(), pCounter);
		fclose(fp);
		
		execl("./bin_adder", xx, yy, zz, (char *)NULL);
		
		free(xx);
		free(yy);
		free(zz);

		xx = NULL;
		yy = NULL;
		zz = NULL;

		exit(EXIT_SUCCESS);

	}
}

//attaches to shared memory
void attachShmem() {

	if((shmId = shmget(key, sizeof(struct SharedMemory), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
                perror("master.cpp: error: failed to allocate shared memory");
                exit(EXIT_FAILURE);
        }
        else {
                shmem = (struct SharedMemory*)shmat(shmId, NULL, 0);
        }

}

//dettaches from shared memory and calls function to delete shared memory
void releaseShmem() {
	if(shmem != NULL) {
		if(shmdt(shmem)) {
			perror("master.cpp: error: failed to release shared memory");
			exit(EXIT_FAILURE);
		}
	}
	
	deleteShmem();

}

//checks to see if id is greater than 0, if so then delete shared memory
void deleteShmem() {	
	if(shmId > 0) {
		if(shmctl(shmId, IPC_RMID, NULL) < 0) {
			perror("master.cpp: error: failed to delete shared memory");
			exit(EXIT_FAILURE);
		}
	}
}

//used to store the integers into shared memory and set default values to pid and bakery algorithm arrays
void storeShmem(vector<int> vect1, int size1) {
	for(int i = 0; i < size1; i++) {
                shmem[i].value = vect1[i];
                shmem[i].pid = 0;
		shmem->choosing[i] = false;
		shmem->number[i] = 0;
        }
}

//used to handle ctrl+c; kills childpids; releases and deletes shared memory; geeksforgeeks source
void signalHandler(int signal) {	

	cout << "master.cpp: exiting: signalHandler" << endl;

	FILE * fp2 = fopen("output.log", "a");
        if(fp2 == NULL) {
        	perror("master.cpp: error: failed to open output.log");
       	}
        fprintf(fp2, "%s \t master.cpp: signalHandler: terminating processes\n", getTime());
        fclose(fp2);

	for(int i = 0; i < shmem->vectSize; i++) {
		kill(shmem[i].pid, SIGQUIT);
	}

	while(wait(NULL)>0);
	releaseShmem();
	
	exit(EXIT_SUCCESS);

}

//returns a time format in hours, minutes, seconds; stackoverflow source
char* getTime() {

	int timeLength = 50;
	char* formatTime = new char[timeLength];
	time_t now = time(NULL);
	
	strftime(formatTime, timeLength, "%H:%M:%S", localtime(&now));
	return formatTime;

}

//sets up the timer to terminate if it exceeds timeout length; triggers signalHandler - periodicasterisk.c
void setTimer(int timeSeconds) {

	struct sigaction act;
	act.sa_handler = signalHandler;
	
	if(sigaction(SIGALRM, &act, NULL) == -1) {
		perror("master.cpp: error: failed to set up sigaction handler for setTimer()");
		exit(EXIT_FAILURE);
	}
	
	struct itimerval value;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;

	value.it_value.tv_sec = timeSeconds;
	value.it_value.tv_usec = timeSeconds;

	if(setitimer(ITIMER_REAL, &value, NULL)) {
		perror("master.cpp: error: failed to set the timer in setTimer()");
		exit(EXIT_FAILURE);
	}

}

//help menu
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

