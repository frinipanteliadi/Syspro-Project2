#define OK 0
#define MEM_ERROR -1
#define FEW_ARG -2
#define NOT_ENOUGH_ARG -3
#define MANY_ARG -4
#define FILE_NOT_OPEN -5
#define EXIT -6

typedef struct map{
	int dirID;
	char* dirPath;
}map;

void printErrorMessage(int);
int CommandLineArg(int);
int getNumberOfLines(FILE*);
int createMap(map**, int);
int initializeMap(FILE*, map*, int);
void printMap(map*, int);
void deleteMap(map**, int);