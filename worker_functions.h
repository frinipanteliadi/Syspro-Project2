#define WORKER_OK 0
#define WORKER_ARGS_ERROR -1
#define WORKER_RAISE_ERROR -2
#define WORKER_OPEN_ERROR -3
#define WORKER_MEM_ERROR -4
#define WORKER_DIR_OPEN_ERROR -5
#define WORKER_DIR_CLOSE_ERROR -6
#define WORKER_EXIT -7

/* False = 0, True = 1 */
typedef enum bool {False, True} bool;

typedef struct postingsListNode{
	char* lineID;
	int occurences;
	struct postingsListNode *next;
}postingsListNode;

typedef struct postingsList{
	int lfVector;
	char* word;
	postingsListNode* headPtr;
}postingsList;

/* Each node of the Trie structure */
typedef struct trieNode{
	char letter;															/*Letter stored inside the node*/
	bool isEndOfWord;														/*Flag which indicated whether a word ends in the current node*/
	struct trieNode* children;												/*Points at a child node*/
	struct trieNode* next;
	postingsList* listPtr;												/*Points at the next sibling node*/
}trieNode;

/* One for each line of a certain file */
typedef struct line_info{
	char* id;																/*ID of the line(form: x|y|z )*/
	char* line_content;														/*The actual line*/
}line_info;

/* One for each file of the directory*/
typedef struct file_info{
	char* file_id;															/*ID of the file (form: x|y)*/
	char* file_name;														/*The name of the file*/
	char* full_path;														/*The full path of the file*/
	int lines;																/*Total number of lines the file has*/
	line_info* ptr;															/*Points at the structure which holds the lines of the file*/
}file_info;

/* One for every directory that was provided */
typedef struct worker_map{
	char* dirID;															/*ID of the directory (form: x)*/																/*ID given to the directory*/
	char* dirPath;															/*The path of the directory*/
	int total_files;														/*Total number of files inside the directory*/
	file_info *dirFiles;													/*Points at the structure which holds the files of the directory*/
}worker_map;


void printWorkerError(int);
int workerArgs(int);

int createWorkerMap(worker_map**, int);
int initializeWorkerMap(worker_map**, int, int, int);
void printWorkerMap(worker_map**, int);
void deleteWorkerMap(worker_map**, int);

int openingPipes(char*, char*, int*, int*);

int getNumberOfLines(FILE*);
int fileInformation(int, worker_map**);

int setLines(int, worker_map**);

int initializeLinePointer(int, worker_map**);

int readLines(int, worker_map**);

int initializeStructs(int, worker_map**);

void printDirectory(int, worker_map**);

int createRoot(trieNode**);

int compareKeys(char*, char*);

int insertTrie(trieNode*, char*, char*);

int initializeTrie(int, worker_map**, trieNode*);

void destroyTrie(trieNode*);
void printNode(trieNode*);
void printTrie(trieNode*);
postingsList* searchTrie(trieNode*, char*);

int addList(postingsList**, char*, char*);
void deleteList(postingsListNode*);
void deletePostingsList(postingsList*);
void printPostingdList(postingsList*, char*);
void printAllLF(trieNode*);

int readPipe(int, int, char**);

int readDirectories(int, int, int, worker_map**);

int getTotalArguments(char*);
int keepArguments(char**, char*);

void getIDs(int*, char*);

void printStructs(int, worker_map**);
