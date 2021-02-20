#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

using namespace std;

const int maxChildProcess = 20;
const int maxTimeSeconds = 100;

void usage();

int main(int argc, char * argv[]) {

	int opt;
	
	int userChildren = 20;
	int userTime = 100;
	
	string fileName;

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
	
	if(argv[optind] == NULL) {
		perror("master.cpp: error: no input file");
		exit(EXIT_FAILURE);
	}
	else {
		fileName = argv[optind];
	}
	

	cout << userChildren << endl;
	cout << userTime << endl;
	cout << fileName << endl;

	return EXIT_SUCCESS;

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

