#define WORKER_OK 0
#define WORKER_ARGS_ERROR -1
#define WORKER_RAISE_ERROR -2
#define WORKER_OPEN_ERROR -3
#define WORKER_MEM_ERROR -4
#define WORKER_DIR_OPEN_ERROR -5
#define WORKER_DIR_CLOSE_ERROR -6
#define WORKER_EXIT -7

typedef enum bool {False, True} bool;

typedef struct trie_node{
	char letter;															// Letter stored in the node
	bool is_end_of_word;													// Flag to indicate that the current letter is a final one
	struct trie_node* children;												// Points at the node's children
	struct trie_node* next;													// Points at the node which is next on the same level
}trie_node;

typedef struct line{
	int line_id;
	char* line_content;
}line;

/* One for each file of the directory*/
typedef struct file_info{
	char* file_name;
	char* full_path;
	int lines;
	line* ptr;
}file_info;

/* Holds every directory that was provided */
typedef struct worker_map{
	int dirID;
	char* dirPath;
	int total_files;
	file_info *dirFiles;
}worker_map;


void printWorkerError(int);
int workerArgs(int);

int createWorkerMap(worker_map**, int);
int initializeWorkerMap(worker_map**, int, int, int);
void printWorkerMap(worker_map**, int);
void deleteWorkerMap(worker_map**, int);

int openingPipes(char*, char*, int*, int*);

int getNumberOfLines(FILE*);