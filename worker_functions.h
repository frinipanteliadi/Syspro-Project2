#define WORKER_OK 0
#define WORKER_ARGS_ERROR -1
#define WORKER_RAISE_ERROR -2
#define WORKER_OPEN_ERROR -3
#define WORKER_MEM_ERROR -4
#define WORKER_EXIT -5

void printWorkerError(int);
int workerArgs(int);