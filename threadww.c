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

typedef enum bool { false = 0, true = 1 } bool;

typedef struct node 
{
    char* dirName;
    char* fileName;
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
    for(node* temp = q->head; temp!=NULL; temp=temp->next)
    {
        if(temp->fileName == NULL)
        {
            printf("dirQueue?: |%s| \n", temp->dirName);
        }
        else if(temp->dirName != NULL)
        {
            printf("fileQueue?: |%s| and |%s| \n", temp->dirName, temp->fileName );
        }
    }
    if(q->head == NULL)
    {
        printf("Nothing in Queue\n");
    }
    else
    {
         printf("------------------------------------\n");
    }
}

void enqueue(char* dirName, char* fileName, queue *q)
{
    pthread_mutex_lock(&q->lock);
    node* curr = (node*) malloc(sizeof(node));
    curr->dirName = dirName;
    curr->fileName = fileName;
    if(q->head == NULL)
    {
        q->head = curr;
        q->rear = curr;
    }
    else
    {
        q->rear->next = curr;
        q->rear = curr;
    }
    q->count++;
    pthread_cond_signal(&q->dequeue_ready);
    pthread_mutex_unlock(&q->lock);
}
void dequeue(node* curr, queue *q)
{
    pthread_mutex_lock(&q->lock);
    while (q->open>0 && q->count == 0) 
    {
        pthread_cond_wait(&q->dequeue_ready, &q->lock);
    }
    memcpy(curr, q->head, sizeof(node));
    q->head = q->head->next;
    if(q->head == NULL)
    {
        q->rear = NULL;
    }
    q->count--;
    pthread_mutex_unlock(&q->lock);
}

int directoryWorker(queue* dir, queue* file)
{
    pthread_mutex_lock(&dir->lock);
    dir->open++;
    pthread_mutex_unlock(&dir->lock);
    if(DEBUG)printf("starting directoryworker \n");

    node* deqNode = (node*)malloc(sizeof(node));
    dequeue(deqNode, dir);

    if(DEBUG)printf("here dequeded %s\n", deqNode->dirName);

    DIR *givenDir = opendir(deqNode->dirName);
    if(givenDir == NULL)
    {
        printf("invalid directory in queue\n");
    }
    struct dirent *currDir;
    currDir = readdir(givenDir);
    struct stat checkDir;

    chdir(deqNode->dirName);

    while(currDir!=NULL)
    {
        if(DEBUG)printf("checking dir: %s \n", currDir->d_name);
        stat(currDir->d_name, &checkDir);
        if(currDir->d_name[0]=='.')//. case
        {
            if(DEBUG)printf("ignored bc of .: %s \n",currDir->d_name);
            currDir = readdir(givenDir);
            continue;
        }
        else if(strlen(currDir->d_name)>5)//.wrap case
        {
            if(currDir->d_name[0]=='w'&&currDir->d_name[1]=='r'&&currDir->d_name[2]=='a'&&currDir->d_name[3]=='p'&&currDir->d_name[4]=='.')
            {
                if(DEBUG)printf("ignored bc of wrap.: %s \n",currDir->d_name);
                currDir = readdir(givenDir);
                continue;
            }
        }
        if(S_ISDIR(checkDir.st_mode))//check if a directory is a folder
        {
            
            int dirStrLen = strlen(deqNode->dirName);
            int tempDirStrLen = strlen(currDir->d_name);
            char* newDir = (char*)malloc(dirStrLen + tempDirStrLen + 2);
            memcpy(newDir, deqNode->dirName, dirStrLen);
            newDir[dirStrLen] = '/';
            memcpy(&newDir[dirStrLen+1], currDir->d_name, tempDirStrLen);
            newDir[dirStrLen+1+tempDirStrLen] = '\0';
            enqueue(newDir, NULL, dir);
            if(DEBUG)printf("adding |%s| to dirQueue\n",newDir);
            currDir = readdir(givenDir);
            continue;
        }
        else if(S_ISREG(checkDir.st_mode))//Check if a directory is file
        {
            if(DEBUG)printf("adding |%s| as dirName and |%s| as fileName to fileQueue\n",deqNode->dirName, currDir->d_name);
            enqueue(deqNode->dirName, currDir->d_name, file);
        }
        currDir = readdir(givenDir);
    }
    closedir(givenDir);

    pthread_mutex_lock(&dir->lock);
    dir->open--;
    pthread_mutex_unlock(&dir->lock);

    return 0;
}

int wwWorker(queue* file, int line_length)
{
    return 0;
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

    queue* directory = (queue*)malloc(sizeof(queue));
    queue_init(directory);
    queue* file = (queue*)malloc(sizeof(queue));
    queue_init(file);
    enqueue("testingDirectory",NULL, directory);
    printQueue(directory);
    directoryWorker(directory, file);
    printQueue(directory);
    printQueue(file);
    return EXIT_SUCCESS;
}