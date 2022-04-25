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

#define DEBUG 0
#define printFailed 0

typedef struct node
{
    char* directory;
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

void printQueue(queue *q)
{
    printf("------------------------------------\n");
    pthread_mutex_lock(&q->lock);
	struct stat st;
    for(node* temp = q->head; temp!=NULL; temp=temp->next)
    {
		stat(temp->directory, &st);
        if(S_ISDIR(st.st_mode))
        {
            printf("dirQueue?: |%s| \n", temp->directory);
        }
        else if(S_ISREG(st.st_mode))
        {
            printf("fileQueue?: |%s| \n", temp->directory);
        }
    }
    if(q->head == NULL)
    {
        printf("Nothing in Queue\n");
    }
    pthread_mutex_unlock(&q->lock);
    printf("------------------------------------\n");
}

void queue_init(queue *q)
{
    if(DEBUG)printf("queue Initialized\n");
    q->head = NULL;
    q->rear = NULL;
    q->open = 0;
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->dequeue_ready, NULL);
    pthread_cond_init(&q->enqueue_ready, NULL);
}

void enqueue(node* n, queue *q)
{
    pthread_mutex_lock(&q->lock);
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
    pthread_mutex_unlock(&q->lock);
}

int dequeue(void* n, queue *q, pthread_t tid)
{
    if(DEBUG)printf("1:|%lu| count: |%d| open|%d|\n",tid,q->count, q->open);
    pthread_mutex_lock(&q->lock);
    if(q->count==0 && q->open ==0)
    {
        if(DEBUG)printf("DUP:|%lu| count: |%d| open|%d|\n",tid,q->count, q->open);
        n = NULL;
        pthread_mutex_unlock(&q->lock);
        return 0;
    }
    if(q->count>0)
    {
        if(DEBUG)printf("2:|%lu| count: |%d| open|%d|\n",tid,q->count, q->open);
        memcpy(n,q->head,sizeof(node));
        q->head = q->head->next;
        q->count--;
        q->open++;
        pthread_mutex_unlock(&q->lock);
        return 1;
    }
    if(q->count==0 && q->open >0)
    {
        if(DEBUG)printf("3:|%lu| count: |%d| open|%d|\n",tid,q->count, q->open);
        while(q->count==0 && q->open >0)
        {
            if(DEBUG)printf("waiting in |%lu|\n", tid);
            pthread_cond_wait(&q->dequeue_ready, &q->lock);
        }
        if(DEBUG)printf("done waiting in |%lu|\n", tid);
        if(q->count==0 && q->open ==0)
        {
            if(DEBUG)printf("4:|%lu| count: |%d| open|%d|\n",tid,q->count, q->open);
            pthread_mutex_unlock(&q->lock);
            return 0;
        }
        if(DEBUG)printf("5:|%lu| count: |%d| open|%d|\n",tid,q->count, q->open);
        memcpy(n,q->head,sizeof(node));
        q->head = q->head->next;
        q->count--;
        q->open++;
        pthread_mutex_unlock(&q->lock);
        return 1;
    }
    if(q->count==0 && q->open ==0)
    {
        if(DEBUG)printf("6:|%lu| count: |%d| open|%d|\n",tid,q->count, q->open);
        n = NULL;
        pthread_mutex_unlock(&q->lock);
        return 0;
    }
    if(DEBUG)printf("7:|%lu| count: |%d| open|%d|\n",tid,q->count, q->open);
    pthread_mutex_unlock(&q->lock);
    if(DEBUG)printf("should not get here\n");
    return -1;
}



typedef struct dwargs {
	pthread_t tid;
	queue* dq;
	queue* fq;
} dwargs;

void* directoryWorker(void* vargs) 
{
    dwargs* args = (dwargs*)vargs;
    node* testing = (node*)malloc(sizeof(node));
    while(dequeue((void*)testing, args->dq, args->tid))
    {
        char* dir = testing->directory;
        DIR* qdir = opendir(dir);
        if(DEBUG)printf("directory popped in |%lu|: |%s|\n", args->tid,dir);
        struct dirent *cdir;
    	cdir = readdir(qdir);
        struct stat st;

        while(cdir!=NULL)
    	{
            int ppathlen = strlen(dir);
        	int cpathlen = strlen(cdir->d_name);
        	char* cpath = (char*)malloc(ppathlen + cpathlen + 2);
        	memcpy(cpath, dir, ppathlen);
        	cpath[ppathlen] = '/';
        	memcpy(&cpath[ppathlen + 1], cdir->d_name, cpathlen);
        	cpath[ppathlen + 1 + cpathlen] = '\0';

            //if(DEBUG) printf("checking %s\n",cpath);

            stat(cpath, &st);
            if(cpathlen == 0 || cdir->d_name[0] == '.' || !strncmp("wrap.", cdir->d_name, 5))
        	{
				//if(DEBUG) printf("ignored |%s| \n",  cdir->d_name);
        	} 
            else if(S_ISDIR(st.st_mode)) 
            { 
                if(DEBUG) printf("add |%s/%s| to dir queue\n", dir, cdir->d_name);
                node* nextDir = (node*)malloc(sizeof(node));
                nextDir->directory = cpath;
                nextDir->next = NULL;
                enqueue(nextDir, args->dq);
            }
            else if(S_ISREG(st.st_mode)) 
            { 
                node* nextFile = (node*)malloc(sizeof(node));
                nextFile->directory = cpath;
                nextFile->next = NULL;
                enqueue(nextFile, args->fq);
                pthread_cond_signal(&args->fq->dequeue_ready);
            }
            cdir = readdir(qdir);
        }
        closedir(qdir);
        
        pthread_mutex_lock(&args->dq->lock);
        args->dq->open--;
        pthread_mutex_unlock(&args->dq->lock);
    }
    free(testing);
    pthread_cond_signal(&args->dq->dequeue_ready);
    pthread_cond_signal(&args->fq->dequeue_ready);
    pthread_exit(NULL);
}

void* fileWorker(void* vargs) 
{
    dwargs* args = (dwargs*)vargs;
    node* testing = (node*)malloc(sizeof(node));
    int temp = 0;
    while(1)
    {
        pthread_mutex_lock(&args->dq->lock);
        while(args->dq->count>0 || args->dq->open>0)
        {
            pthread_cond_wait(&args->fq->dequeue_ready, &args->dq->lock);
        }

        pthread_mutex_lock(&args->fq->lock);
        if(args->dq->count==0 && args->dq->open==0 && args->fq->count==0)
        {
            pthread_mutex_unlock(&args->fq->lock);
            pthread_mutex_unlock(&args->dq->lock);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&args->fq->lock);
        temp = dequeue((void*)testing, args->fq, args->tid);

        pthread_mutex_lock(&args->fq->lock);
        args->fq->open--;
        pthread_mutex_unlock(&args->fq->lock);

        pthread_cond_signal(&args->fq->dequeue_ready);
        pthread_mutex_unlock(&args->dq->lock);

        if(testing == NULL)
        {
            pthread_exit(NULL);
        }
        if(testing->directory == NULL)
        {
            printf("ERROR: file directory is NULL\n");
        }
        printf("popped |%s| from the file queue so do stuff with it\n", testing->directory);
    }
}

int main(int argc, char** argv) 
{
    if(argc !=3)
    {
        printf("not enough parameters\n");
        return EXIT_SUCCESS;
    }
    queue* dq = (queue*)malloc(sizeof(queue));
    queue_init(dq);
    queue* fq = (queue*)malloc(sizeof(queue));
    queue_init(fq);

    char* path = {"testingDirectory"};
    node* testing = (node*)malloc(sizeof(node));
    testing->directory = path;
    testing->next = NULL;

    enqueue(testing, dq);
    printQueue(dq);


    dwargs* qs = (dwargs*)malloc(sizeof(dwargs));
    qs->dq = dq;
    qs->fq = fq;


    pthread_t* dwtids = (pthread_t*)malloc(atoi(argv[1]) * sizeof(pthread_t));
    dwargs** da = malloc(atoi(argv[1]) * sizeof(dwargs*));
	for(int i = 0; i < atoi(argv[1]); i++) 
    {
		dwargs* di = malloc(sizeof(dwargs));
		//da[i]->dq = dq;
		//da[i]->fq = fq;
		//da[i]->tid = i;
		di->dq = dq;
		di->fq = fq;
		di->tid = i;
		da[i] = di;
	}

    for(int i = 0; i < atoi(argv[1]); i++) 
    {
		pthread_create(&dwtids[i], NULL, directoryWorker, (void*)da[i]);
        if(DEBUG)printf("starting dw %lu\n",dwtids[i]);
	}

    pthread_t* fwtids = (pthread_t*)malloc(atoi(argv[2]) * sizeof(pthread_t));
    // dwargs** fa = malloc(atoi(argv[2]) * sizeof(dwargs*));
	// for(int i = 0; i < atoi(argv[2]); i++) 
    // {
	// 	dwargs* di = malloc(sizeof(dwargs));
	// 	//da[i]->dq = dq;
	// 	//da[i]->fq = fq;
	// 	//da[i]->tid = i;
	// 	di->dq = dq;
	// 	di->fq = fq;
	// 	di->tid = i;
	// 	da[i] = di;
	// }

    for(int i = 0; i < atoi(argv[2]); i++) 
    {
		pthread_create(&fwtids[i], NULL, fileWorker, (void*)qs);
        if(DEBUG)printf("starting fw %lu\n",fwtids[i]);
	}

    printf("done\n");
    for(int i = 0; i < atoi(argv[1]); i++) 
    {
        if(DEBUG)printf("---attemping dw to join%lu\n",dwtids[i]);
		pthread_join(dwtids[i], NULL);
        if(DEBUG)printf("---joined dw %lu\n",dwtids[i]);
	}
	printf("Dir workers done\n");

    for(int i = 0; i < atoi(argv[2]); i++) 
    {
        if(DEBUG)printf("---attemping to join fw %lu\n",fwtids[i]);
		pthread_join(fwtids[i], NULL);
        if(DEBUG)printf("---joined fw %lu\n",fwtids[i]);
	}
	printf("file workers done\n");


    printQueue(fq);
    return EXIT_SUCCESS;
}