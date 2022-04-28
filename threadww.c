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

typedef struct node {
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

void queue_init(queue *q) {
    q->head = NULL;
    q->rear = NULL;
    q->open = 0;
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->dequeue_ready, NULL);
}

void printQueue(queue *q) {
    pthread_mutex_lock(&q->lock);
	struct stat st;
    for(node* temp = q->head; temp!=NULL; temp=temp->next) {
		stat(temp->path, &st);
        if(S_ISDIR(st.st_mode)) {
            printf("dirQueue?: |%s| \n", temp->path);
        }
        else if(S_ISREG(st.st_mode))
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

void enqueue(node* n, queue *q) {
    if(q->head == NULL) {
        q->head = n;
        q->rear = n;
    } else {
        q->rear->next = n;
        q->rear = n;
    }
    q->count++;
    pthread_cond_signal(&q->dequeue_ready);

}

void dequeue(node** n, queue *q) {
    *n = q->head;
    q->head = q->head->next;
    q->count--;
}

typedef struct dwargs {
	int tid;
	queue* dq;
	queue* fq;
} dwargs;

void* directoryworker(void* vargs) {
	int *ret = malloc(sizeof(int));
	*ret = 0;
	dwargs* args = (dwargs*)vargs;
    while(true) {
		pthread_mutex_lock(&args->dq->lock);
		if(args->dq->open == 0 && args->dq->count == 0) {
			pthread_cond_signal(&args->dq->dequeue_ready);
			pthread_mutex_unlock(&args->dq->lock);

			pthread_mutex_lock(&args->fq->lock);
			pthread_cond_signal(&args->fq->dequeue_ready);
			pthread_mutex_unlock(&args->fq->lock);

			pthread_exit((void*)ret);
			return (void*)ret;
		}
		while(args->dq->count == 0 && args->dq->open > 0) {
			pthread_cond_wait(&args->dq->dequeue_ready, &args->dq->lock);
		}
		if(args->dq->open == 0 && args->dq->count == 0) {
			pthread_cond_signal(&args->dq->dequeue_ready);
			pthread_mutex_unlock(&args->dq->lock);

			pthread_mutex_lock(&args->fq->lock);
			pthread_cond_signal(&args->fq->dequeue_ready);
			pthread_mutex_unlock(&args->fq->lock);

			pthread_exit((void*)ret);
			return (void*)ret;
		}
		node* deqNode = (node*)malloc(sizeof(node));
    	dequeue(&deqNode, args->dq);
		args->dq->open++;
    	pthread_mutex_unlock(&args->dq->lock);

		if(deqNode==NULL) {
			pthread_mutex_unlock(&args->dq->lock);
			return (void*)ret;
    	}
		struct stat st;
		stat(deqNode->path, &st);
		if(S_ISREG(st.st_mode)) {
			node* n = (node*)malloc(sizeof(node));
			n->path = deqNode->path;
			n->next = NULL;

			pthread_mutex_lock(&args->fq->lock);
			enqueue(n, args->fq);
			pthread_cond_signal(&args->fq->dequeue_ready);
			pthread_mutex_unlock(&args->fq->lock);

			pthread_mutex_lock(&args->dq->lock);
    		args->dq->open--;
    		pthread_mutex_unlock(&args->dq->lock);
			continue;
		}

    	DIR* qdir = opendir(deqNode->path);

    	if(qdir == NULL) {
        	printf("ERROR: invalid directory in queue of |%s| \n", deqNode->path);
			return NULL;
    	}

    	struct dirent *cdir;
    	cdir = readdir(qdir);

    	while(cdir!=NULL) {
        	int ppathlen = strlen(deqNode->path);
        	int cpathlen = strlen(cdir->d_name);
        	char* cpath = (char*)malloc(ppathlen + cpathlen + 2);
        	memcpy(cpath, deqNode->path, ppathlen);
        	cpath[ppathlen] = '/';
        	memcpy(&cpath[ppathlen + 1], cdir->d_name, cpathlen);
        	cpath[ppathlen + 1 + cpathlen] = '\0';

        	stat(cpath, &st);
        	if(cpathlen == 0 || cdir->d_name[0] == '.' || !strncmp("wrap.", cdir->d_name, 5)) {
				// Ignore file
        	} else if(S_ISDIR(st.st_mode)) { //check if a directory is a folder
            	node* n = (node*)malloc(sizeof(node));
				n->path = cpath;
				n->next = NULL;

				pthread_mutex_lock(&args->dq->lock);
				enqueue(n, args->dq);
				pthread_cond_signal(&args->dq->dequeue_ready);
				pthread_mutex_unlock(&args->dq->lock);
        	} else if(S_ISREG(st.st_mode)) { //Check if a directory is file
            	node* n = (node*)malloc(sizeof(node));
				n->path = cpath;
				n->next = NULL;

				pthread_mutex_lock(&args->fq->lock);
				enqueue(n, args->fq);
				pthread_cond_signal(&args->fq->dequeue_ready);
				pthread_mutex_unlock(&args->fq->lock);
        	}
        	cdir = readdir(qdir);
    	}
    	closedir(qdir);

    	pthread_mutex_lock(&args->dq->lock);
    	args->dq->open--;
    	pthread_mutex_unlock(&args->dq->lock);
    }
    return (void*)ret;
}

typedef struct fwargs {
	int tid;
	queue* fq;
	queue* dq;
	int line_len;
} fwargs;

void* fileworker(void* vargs) {
	fwargs* args = (fwargs*)vargs;
	int *ret = malloc(sizeof(int));
	*ret = 0;
	char dirsempty = 0;
	while(1) {
		pthread_mutex_lock(&args->dq->lock);
		if(args->dq->count == 0 && args->dq->open == 0) dirsempty = 1;
		pthread_mutex_unlock(&args->dq->lock);

		pthread_mutex_lock(&args->fq->lock);
		if(args->fq->count == 0 && dirsempty == 1) {
			pthread_cond_signal(&args->fq->dequeue_ready);
			pthread_mutex_unlock(&args->fq->lock);
			pthread_exit((void*)ret);
			return ret;
		}
		while(args->fq->count == 0 && args->fq->open > 0) {
			pthread_cond_wait(&args->fq->dequeue_ready, &args->fq->lock);
		}
		if(args->fq->count == 0) {
			pthread_cond_signal(&args->fq->dequeue_ready);
			pthread_mutex_unlock(&args->fq->lock);
			pthread_exit((void*)ret);
			return ret;
		}
		args->fq->open++;
    	node* deqNode = (node*)malloc(sizeof(node));
    	dequeue(&deqNode, args->fq);
		pthread_mutex_unlock(&args->fq->lock);

		char wrappre[5] = "wrap.";
		int pathlen = strlen(deqNode->path), ls = -1;
		char* writepath = (char*)malloc((pathlen + 6));
		for(int i = 0; i < pathlen; i++) {
			writepath[i] = deqNode->path[i];
			if(writepath[i] == '/') ls = i;
		}
		memcpy(&writepath[ls + 1], &wrappre, 5);
		memcpy(&writepath[ls + 6], &deqNode->path[ls + 1], pathlen - ls + 2);

    	int fdr = open(deqNode->path, O_RDONLY);
    	int fdw = open(writepath, O_RDWR | O_CREAT |O_TRUNC, S_IRUSR|S_IWUSR);
		int w = wrap(fdr, fdw, args->line_len);
    	if(w == 1) *ret = 1;

    	close(fdr);
    	close(fdw);
    	free(deqNode);
		free(writepath);

    	pthread_mutex_lock(&args->fq->lock);
    	args->fq->open--;
    	pthread_mutex_unlock(&args->fq->lock);
	}
    return ret;
}

void dtoq(char* dpath, queue* q) {
	struct stat st;
	stat(dpath, &st);
	DIR* dir = opendir(dpath);

   	struct dirent *cdir;
   	cdir = readdir(dir);

   	while(cdir!=NULL) {
       	int ppathlen = strlen(dpath);
       	int cpathlen = strlen(cdir->d_name);
       	char* cpath = (char*)malloc(ppathlen + cpathlen + 2);
       	memcpy(cpath, dpath, ppathlen);
       	cpath[ppathlen] = '/';
       	memcpy(&cpath[ppathlen + 1], cdir->d_name, cpathlen);
       	cpath[ppathlen + 1 + cpathlen] = '\0';
        stat(cpath, &st);
       	if(cpathlen == 0 || cdir->d_name[0] == '.' || !strncmp("wrap.", cdir->d_name, 5))
       	{

       	} else if(S_ISREG(st.st_mode)) { //Check if a directory is file
           	node* n = (node*)malloc(sizeof(node));
			n->path = cpath;
			n->next = NULL;

			pthread_mutex_lock(&q->lock);
			enqueue(n, q);
			pthread_cond_signal(&q->dequeue_ready);
			pthread_mutex_unlock(&q->lock);
       	}
       	cdir = readdir(dir);
   	}
   	closedir(dir);
}

int main(int argc, char** argv) {
    if(argc < 3) {
        printf("ERROR: Invalid number of parameters\n");
        return EXIT_FAILURE;
    }

	queue* dq = (queue*)malloc(sizeof(queue));
    queue_init(dq);
    queue* fq = (queue*)malloc(sizeof(queue));
    queue_init(fq);
	bool recursive;

    int comma = -1;
    int M = 0;
    int N = 1;
	int line_length;
	struct stat st;

	if (strncmp("-r", argv[1], 2) == 0 || strncmp("-R", argv[1], 2) == 0) {
		recursive = true;
		int strlength = strlen(argv[1]);
    	line_length = atoi(argv[2]);
    	char* dirPath = argv[3];
    	if(strlength==2) {
    	    M = 1;
    	    N = 1;
    	    if(DEBUG) printf("running -r1,1 \n");
    	} else {
    	    int i;
        	for(i = 2; i < strlength;i++) {
        	    if(argv[1][i]==',') {
        	        comma = i;
        	        break;
        	    }
        	}
        	if(comma == -1) {
         	    char* Ntemp = (char*)malloc(sizeof(char)*(strlength-1));
         	    memcpy(Ntemp,&argv[1][2],strlength-1);
				N = atoi(Ntemp);
            	if(DEBUG) printf("running -r%d since N is %d \n", N, N);
            	free(Ntemp);
        	} else {
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
		if(argc == 4) {
			stat(argv[3], &st);
			if(S_ISREG(st.st_mode)) {
				int f = open(argv[3], O_RDONLY);
				int ret = wrap(f, 1, line_length);
				close(f);
				free(dq);
				free(fq);
				return ret;
			}
		}
		for(int i = 3; i < argc; i++) {
			node* f = malloc(sizeof(node));
			f->path = argv[i];
			f->next = NULL;
			enqueue(f, dq);
		}
	} else {
		recursive = false;
		line_length = atoi(argv[1]);
		if(argc == 3) {
			stat(argv[2], &st);
			if(S_ISREG(st.st_mode)) {
				int f = open(argv[2], O_RDONLY);
				int ret = wrap(f, 1, line_length);
				close(f);
				free(dq);
				free(fq);
				return ret;
			}
		}
		for(int i = 2; i < argc; i++) {
			stat(argv[i], &st);
			if(S_ISREG(st.st_mode)) {
				node* f = malloc(sizeof(node));
				f->path = argv[i];
				f->next = NULL;
				enqueue(f, fq);
			} else if(S_ISDIR(st.st_mode)) {
				dtoq(argv[i], fq);
			}
		}
	}

	pthread_t* dwtids;
	dwargs** da;
	if(M > 0) {
		// Start Directory worker threads
		dwtids = malloc(M * sizeof(pthread_t));
		da = malloc(M * sizeof(dwargs*));
	}
	for(int i = 0; i < M; i++) {
		dwargs* di = malloc(sizeof(dwargs));
		di->dq = dq;
		di->fq = fq;
		di->tid = i;
		da[i] = di;
	}

	for(int i = 0; i < M; i++) {
		pthread_create(&dwtids[i], NULL, directoryworker, da[i]);
	}

	//Start Fileworker Threads
	pthread_t* fwtids = malloc(N * sizeof(pthread_t));
	fwargs* fa = malloc(sizeof(fwargs));
	fa->fq = fq;
	fa->dq = dq;
	fa->line_len = line_length;
	for(int i = 0; i < N; i++) {
		pthread_create(&fwtids[i], NULL, fileworker, fa);
	}

	unsigned int status = 0;
	void* s;
	// Join threads
	for(int i = 0; i < M; i++) {
		pthread_join(dwtids[i], &s);
		if(*((int *)s) == 1) {
			status = 1;
		}
		free(s);
	}

	for(int i = 0; i < N; i++) {
		pthread_join(fwtids[i], &s);
		if(*((int *)s) == 1) {
			status = 1;
		}
		free(s);
	}

	return status;
}
