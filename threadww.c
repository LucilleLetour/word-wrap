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
    if(q->head->next ==NULL)
    {
        free(q->head);
        q->head == NULL;
    }
    q->head = q->head->next;
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
        printf("invalid directory in queue of |%s| \n", deqNode->dirName);
    }
    struct dirent *currDir;
    currDir = readdir(givenDir);
    struct stat checkDir;

    while(currDir!=NULL)
    {
        int dirStrLen = strlen(deqNode->dirName);
        int tempDirStrLen = strlen(currDir->d_name);
        char* newDir = (char*)malloc(dirStrLen + tempDirStrLen + 2);
        memcpy(newDir, deqNode->dirName, dirStrLen);
        newDir[dirStrLen] = '/';
        memcpy(&newDir[dirStrLen+1], currDir->d_name, tempDirStrLen);
        newDir[dirStrLen+1+tempDirStrLen] = '\0';

        if(DEBUG)printf("checking dir: %s \n", newDir);
        stat(newDir, &checkDir);
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
    pthread_mutex_lock(&file->lock);
    file->open++;
    pthread_mutex_unlock(&file->lock);
    node* deqNode = (node*)malloc(sizeof(node));
    dequeue(deqNode, file);
    
    int pathlen = strlen(deqNode->dirName);
    int filelen = strlen(deqNode->fileName);

    char* pathToOpen = (char*)malloc(sizeof(char*)*(pathlen + filelen + 2));
    char* pathToWrite = (char*)malloc(sizeof(char*)*(pathlen + filelen + 2 + 5));

    memcpy(pathToOpen, deqNode->dirName, pathlen);
    pathToOpen[pathlen] = '/';
    memcpy(&pathToOpen[pathlen+1], deqNode->fileName, filelen);
    pathToOpen[pathlen + filelen + 1] = '\0';

    memcpy(pathToWrite, deqNode->dirName, pathlen);
    char wrapCon[6] = "/wrap.";
    memcpy(&pathToWrite[pathlen], &wrapCon, 6);
    memcpy(&pathToWrite[pathlen+6], deqNode->fileName, filelen);
    pathToOpen[pathlen + filelen + 6] = '\0';

    printf("open file |%s| and write to |%s|\n", pathToOpen, pathToWrite);
    int fdr = open(pathToOpen, O_RDONLY);
    int fdw = open(pathToWrite, O_RDWR | O_CREAT |O_TRUNC, S_IRUSR|S_IWUSR);
    int ret = wrap(fdr, fdw, line_length);

    close(fdr);
    close(fdw);

    free(pathToOpen);
    free(pathToWrite);
    free(deqNode);

    pthread_mutex_lock(&file->lock);
    file->open--;
    pthread_mutex_unlock(&file->lock);

    return ret;
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
    directoryWorker(directory, file);
    printQueue(directory);
    printQueue(file);

    wwWorker(file, 10);
    printQueue(file);
    char buff[FILENAME_MAX];
    return EXIT_SUCCESS;
}