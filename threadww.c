#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

#define DEBUG 1
#define printFailed 0

typedef enum bool { false = 0, true = 1 } bool;

int wrap(int fdr, int fdw, int line_length)
{
	int pstage = 0;
	char c[1];
	char word[5192];
	int wordlen = 0;
	int linelen = 0;
	int pos = 0;
	bool failed = false;
	bool ins_space = false;
	char lst, llst;
	while (read(fdr, c, 1)) {
		if (c[0] == ' ' || c[0] == '\n') { // Handle ws
			if (wordlen > 0) { // Add the word that's been being built first
				if (wordlen > line_length) { // The word is longer than the wrap length; report failure
					failed = true;
				}
				if (pos > 0 && linelen + wordlen + (ins_space ? 1 : 0) > line_length && lst != '\n') { // Word (when added) will exceed wrap length, so add newline (unless it is the first word)
					write(fdw, "\n", 1);
					pos++;
					linelen = 0;
					ins_space = false;
				}
				if(ins_space) {
					write(fdw, " ", 1);
					pos++;
					linelen++;
				}
				for (int i = 0; i < wordlen; i++) { // Add in the word
					write(fdw, word + i, 1);
					pos++;
					linelen++;
				}
				wordlen = 0;
			}
			if (pos == 0) continue;

			if (c[0] == '\n') {
				if(pstage == 1) { // Previous character was also newline, must make new paragraph
					write(fdw, "\n", 1);
					pos++;
					if(llst != '\n') {
						write(fdw, "\n", 1);
						pos++;
					}
					ins_space = false;
					linelen = 0;
					pstage = 2; // Directly proceding newlines are ignored
				} else if(pstage == 0) {
					pstage = 1;
					ins_space = true;
				}
			} else if (c[0] == ' ') {
				if (pos != 0 && lst != ' ' && lst != '\n' && linelen <= line_length) { // Add space char if appropriate: no preceding whitespace and not overrunning line length
					ins_space = true;
				}
			}
		} else {
			if(pstage == 1) {
				if(c[0] != ' ' && lst != ' ' && lst != '\n') { // Add a space instead of the newline, if it's needed
					write(fdw, " ", 1);
					pos++;
					linelen++;
				}
			}
			pstage = 0;
			word[wordlen] = c[0];
			wordlen++;
		}
		llst = lst;
		lst = c[0];
	}
	if (wordlen > 0) { // Repeat of above, add word into file in case EOF with word in buffer
		if (wordlen > line_length) {
			failed = true;
		}
		if (linelen + wordlen > line_length) {
			write(fdw, "\n", 1);
			linelen = 0;
			pos++;
		}
		for (int i = 0; i < wordlen; i++) {
			write(fdw, word + i, 1);
			pos++;
			linelen++;
		}
		wordlen = 0;
	}
	if(pos > 0) {
		write(fdw, "\n", 1);
		pos++;
	}
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

typedef struct node
{
    char* path;
    struct node* next;
} node;

typedef struct queue {
    node* head;
    node* rear;
    int open;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t enqueue_ready, dequeue_ready;
} queue;

void queue_init(queue *q)
{
    if(DEBUG)printf("queue Initialized\n");
    q->head = NULL;
    q->rear = NULL;
    q->open = 0;
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->dequeue_ready, NULL);
}

void printQueue(queue *q)
{
    printf("------------------------------------\n");
    pthread_mutex_lock(&q->lock);
    if(DEBUG)printf("locked \n");
	struct stat st;
    for(node* temp = q->head; temp!=NULL; temp=temp->next)
    {
		stat(temp->path, &st);
        if(st.st_mode == IS_DIR)
        {
            printf("dirQueue?: |%s| \n", temp->path);
        }
        else if(st.st_mode == IS_REG)
        {
            printf("fileQueue?: |%s| \n", temp->path);
        }
    }
    if(q->head == NULL)
    {
        printf("Nothing in Queue\n");
    }
    if(DEBUG)printf("unlocked \n");
    pthread_mutex_unlock(&q->lock);
    printf("------------------------------------\n");
}

void enqueue(node* n, queue *q)
{
    if(q->head == NULL)
    {
        q->head = n;
        q->rear = n;
    }
    else
    {
        q->rear->next = n;
        q->rear = n;
    }
    q->count++;
    pthread_cond_signal(&q->dequeue_ready);

}
void dequeue(node** n, queue *q)
{
    *n = q->head;
    q->head = q->head->next;
    q->count--;
}

typedef struct dwargs {
	queue* dq;
	queue* fq;
} dwargs;


//I dont think its a good idea to unlock before dequeue
void* directoryworker(void* vargs) {
	dwargs* args = (dwargs*)vargs;
    int locked == pthread_mutex_trylock(&args->dq->lock);
    while(!(args->dq->open == 0 && args->dq->count == 0)) 
	{
		while(args->dq->count == 0) {
			pthread_cond_wait(&args->dq->dequeue_ready, &args->dq->lock)
		}
		if(DEBUG)printf("there is a dir to read \n");

		//chech for cond here
		node* deqNode = (node*)malloc(sizeof(node));
    	dequeue(deqNode, dir);
		args->dq->open++;
    	pthread_mutex_unlock(&args->dq->lock);

		if(deqNode==NULL) 
		{
        	if(DEBUG) printf("there is something wrong\n");
			return &EXIT_FAILURE;
    	}

    	if(DEBUG) printf("here dequeded %s\n", deqNode->path);

    	DIR* qdir = opendir(deqNode->path);

    	if(qdir == NULL)
    	{
        	printf("invalid directory in queue of |%s| \n", deqNode->path);
    	}

    	struct dirent *cdir;
    	cdir = readdir(qdir);
    	struct stat st;

    	while(cdir!=NULL)
    	{
        	int ppathlen = strlen(deqNode->path);
        	int cpathlen = strlen(cdir->d_name);
        	char* cpath = (char*)malloc(ppathlen + cpathlen + 2);
        	memcpy(cpath, deqNode->path, ppathlen);
        	cpath[ppathlen] = '/';
        	memcpy(&cpath[ppathlen + 1], cdir->d_name, cpathlen);
        	cpath[ppathlen + 1 + cpathlen] = '\0';

        	stat(cpath, &st);
        	if(cpathlen == 0 || cdir->d_name[0] == '.' || !strncmp("wrap.", cdir->d_name, 5))
        	{
            //if(1)printf("ignored bc there is no name: |%s| \n",currDir->d_name);
            //cdir = readdir(qdir);
            //continue;
        	//} else if(currDir->d_name[0]=='.') {//. case
            	//if(1)printf("ignored bc of .: %s \n",currDir->d_name);
            	//cdir = readdir(qdir);
            	//continue;
        	//} else if(!strncmp("wrap.", cdir->d_name, 5)) {
            	//if(1)printf("ignored bc of wrap.: %s \n",currDir->d_name);
            	//cdir = readdir(qdir);
            	//continue;
				if(DEBUG) printf("%s\n", cdir->d_name);
        	} else if(S_ISDIR(st.st_mode)) { //check if a directory is a folder
            	//if(1)printf("adding |%s| to dirQueue\n",newDir);
            	node* n = (node*)malloc(sizeof(node));
				n->path = cpath;
				n->next = NULL;

				pthread_mutex_lock(&args->dq->lock);
				enqueue(n, args->dq);
				pthread_mutex_unlock(&args->dq->lock);

        	} else if(S_ISREG(st.st_mode)) { //Check if a directory is file
            	//if(1)printf("adding |%s| as dirName and |%s| as fileName to fileQueue\n",deqNode->path, cdir->d_name);
            	node* n = (node*)malloc(sizeof(node));
				n->path = cpath;
				n->next = NULL;

				pthread_mutex_lock(&args->fq->lock);
				enqueue(n, args->fq);
				pthread_mutex_unlock(&args->fq->lock);
        	}
        	cdir = readdir(qdir);
    	}
    	closedir(qdir);

    	pthread_mutex_lock(&args->dq->lock);
    	if(DEBUG)printf("locked \n");
    	args->dq->open--;
    	if(DEBUG)printf("unlocked \n");
    	pthread_mutex_unlock(&args->dq->lock);
    }
    if(DEBUG)printf("unlocked \n");
    if(locked == 0) pthread_mutex_unlock(&args->dq->lock);
    return &EXIT_SUCCESS;
}

typedef struct fwargs {
	queue* fq;
	int line_len;
} fwargs;

void* fileworker(void* vargs) {
	fwargs* args = (fwargs*)vargs;
    int locked = pthread_mutex_trylock(&args->fq->lock);
    int ret = EXIT_SUCCESS;
	/*
	Change logic to:
	wait for dequeue ready
	check if open == 0 and count ==0
		if true return (we are done)
		otherwise read from queue and continue
	*/
	while(!(args->fq->open == 0 && args->fq->count == 0)) {
		locked = 1;
		while(args->fq->count == 0) {
			pthread_cond_wait(&args->fq->dequeue_ready, &args->fq->lock);
		}
		args->fq->open++;
    	pthread_mutex_unlock(&args->fq->lock);
    	node* deqNode = (node*)malloc(sizeof(node));
    	dequeue(&deqNode, args->fq);

		char wrappre[5] = "wrap.";
		int pathlen = strlen(deqNode->path), ls;
		char writepath[pathlen + 6];
		for(int i = 0; i < pathlen; i++) {
			writepath[i] = deqNode->path;
			if(writepath[i] == '/') ls = i;
		}
		memcpy(&writepath[ls + 1], &wrappre, 5);
		memcpy(&writepath[ls + 6], &deqNode->path[ls + 1], pathlen - ls + 2); // Does this add \0?
		//writepath[pathlen + 6] = '\0';

    	printf("open file |%s| and write to |%s|\n", pathToOpen, pathToWrite);
    	int fdr = open(deqNode->path, O_RDONLY);
    	int fdw = open(writepath, O_RDWR | O_CREAT |O_TRUNC, S_IRUSR|S_IWUSR);
    	ret = ret | wrap(fdr, fdw, args->line_len);

    	close(fdr);
    	close(fdw);
    	free(deqNode);

    	pthread_mutex_lock(&args->fq->lock);
    	args->fq->open--;
    	pthread_mutex_unlock(&args->fq->lock);
	}
	if(locked == 0) pthread_mutex_unlock(&args->fq->lock);
    return &ret;
}


bool updateCond(queue *dir) {
    bool ret = false;
    pthread_mutex_lock(&dir->lock);
    printf("locked\n");
    if(dir->count>0||dir->open>0)
    {
        ret = true;
    }
    printf("unlocked\n");
    pthread_mutex_unlock(&dir->lock);
    return ret;
}

int wwThreader(int N, int line_length, queue dir, queue file)
{
    return 1;
}

int main(int argc, char** argv) 
{
    if(argc != 4)
    {
        printf("ERROR: Invalid number of parameters\n");
        return EXIT_FAILURE;
    }
    int comma = -1;
    int M = -1;
    int N = -1;
    int strlength = strlen(argv[1]);
    int line_length = atoi(argv[2]);
    char* dirPath = argv[3];
    if(strlength==2)
    {
        M = 1;
        N = 1;
        if(DEBUG) printf("running -r1,1 \n");
    }
    else
    {
        int i;
        for(i = 2; i < strlength;i++)
        {
            if(argv[1][i]==',')
            {
                comma = i;
                break;
            }
        }
        if(comma == -1)
        {
            char* Ntemp = (char*)malloc(sizeof(char)*(strlength-1));
            memcpy(Ntemp,&argv[1][2],strlength-1);
            N = atoi(Ntemp);
            if(DEBUG) printf("running -r%d since N is %d \n", N, N);
            free(Ntemp);
        }
        else
        {
            char* Mtemp = (char*)malloc(sizeof(char)*(i-1));
            char* Ntemp = (char*)malloc(sizeof(char)*(strlength-i+1));
            memcpy(Mtemp,&argv[1][2],i-1);
            memcpy(Ntemp,&argv[1][i+1],strlength-i+1);
            Mtemp[i-1] = '\0';
            M = atoi(Mtemp);
            N = atoi(Ntemp);
            if(DEBUG) printf("running -r%d,%d since M is %d and N is %d \n", M,N, M,N);
            free(Mtemp);
            free(Ntemp);
        }
    }

    queue* dq = (queue*)malloc(sizeof(queue));
    queue_init(dq);
    queue* fq = (queue*)malloc(sizeof(queue));
    queue_init(fq);

	node* f = malloc(sizeof(node));
	f->path = argv[3];
	f->next = NULL;
	enqueue(f, fq);

	// Start Directory worker threads
	pthread_t* dwtids = malloc(M * sizeof(pthread_t));
	dwargs* da = malloc(sizeof(dwargs));
	da->dq = dq;
	da->fq = fq;

	for(int i = 0; i , M; i++) {
		pthread_create(&dwtids[i], NULL, directoryworker, da)
	}

	//Start Fileworker Threads
	pthread_t* fwtids = malloc(N * sizeof(pthread_t));
	fwargs* fa = malloc(sizeof(fwargs));
	fa->fq = fq;
	fa->line_len = line_length;

	for(int i = 0; i < N; i++) {
		pthread_create(&fwtids[i], NULL, fileworker, fa);
	}

	int status = EXIT_SUCCESS;
	int s;
	// Join threads
	for(int i = 0; i < M; i++) {
		pthread_join(dwtids[i], &s);
		status = status | s;
	}

	for(int i = 0; i < N; i++) {
		pthread_join(fwtids[i], &s);
		status = status | s;
	}
	return status;

    //enqueue("testingDirectory\0",NULL, directory);

    // node* nodeTemp = (node*)malloc(sizeof(node));
    // printQueue(directory);
    // enqueue("1","1", directory);
    // printQueue(directory);
    // enqueue("2","2", directory);
    // printQueue(directory);

    // dequeue(nodeTemp, directory);
    // printQueue(directory);

    // enqueue("3","3", directory);
    // printQueue(directory);
    // enqueue("4","4", directory);
    // printQueue(directory);

    // dequeue(nodeTemp, directory);
    // printQueue(directory);
    // enqueue("5","5", directory);
    // printQueue(directory);
    // dequeue(nodeTemp, directory);
    // printQueue(directory);

    




    // printQueue(directory);
    // printf("\n \n");
    // dirThreader(M, directory, file);
    // printQueue(directory);
    // printQueue(file);









    /*printf("init\n");
    printQueue(directory);
    printQueue(file);

    printf("1\n");
    directoryWorker(directory, file);
    printQueue(directory);
    printQueue(file);

    printf("2\n");
    directoryWorker(directory, file);
    printQueue(directory);
    printQueue(file);

    printf("3\n");
    directoryWorker(directory, file);
    printQueue(directory);
    printQueue(file);
    
    printf("4\n");
    directoryWorker(directory, file);
    printQueue(directory);
    printQueue(file);

    printf("5\n");
    directoryWorker(directory, file);
    printQueue(directory);
    printQueue(file);

    printf("6\n");
    directoryWorker(directory, file);
    printQueue(directory);
    printQueue(file);*/
}
