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

#define DEBUG 1
#define printFailed 0

typedef enum bool { false = 0, true = 1 } bool;

typedef struct node 
{
    char* dirName;
    char* fileName;
    struct node* next;
} node;

typedef struct queue {
    node* head;
    bool open;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t enqueue_ready, dequeue_ready;
} queue;

void queue_init(struct queue *q)
{
    q->head = NULL;
    q->open = false;
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->enqueue_ready, NULL);
    pthread_cond_init(&q->dequeue_ready, NULL);
}

void enqueue(char* dirName, char* fileName, queue *q)
{
    pthread_mutex_lock(&q->lock);
    node* curr = (node*) malloc(sizeof(node));
    curr->dirName = dirName;
    curr->fileName = fileName;
    curr->next = q->head;
    q->head = curr;
    q->count++;
    pthread_cond_signal(&q->dequeue_ready);
    pthread_mutex_unlock(&q->lock);
}
void dequeue(node* curr, queue *q)
{
    pthread_mutex_lock(&q->lock);
    while (q->open==true && q->count == 0) 
    {
        pthread_cond_wait(&q->dequeue_ready, &q->lock);
    }
    curr = q->head;
    q->head = q->head->next;
    q->count--;
    pthread_mutex_unlock(&q->lock);
}

int main(int argc, char** argv) 
{
    if(argc != 2)
    {
        printf("ERROR: Invalid number of parameters\n");
        return EXIT_FAILURE;
    }
    int comma = -1;
    int M = -1;
    int N = -1;
    int strlength = strlen(argv[1]);
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
}