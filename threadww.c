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