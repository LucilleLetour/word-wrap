#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>


#define DEBUG 1
#define BUFFER 10

char* wrap(FILE* f) 
{
    return NULL;
}

int writeWW(char* word, char* dirName)
{
    if(word == NULL || dirName == NULL)
    {
        return -1;
    }
    if(DEBUG)printf("file name: %s \n", dirName);
    int wd = open(dirName, O_RDWR | O_CREAT |O_TRUNC, S_IRUSR|S_IWUSR);
    if(wd==-1)
    {  
        perror(dirName);
        return -1;
    }
    int length = strlen(word);
    char writeBuffer[BUFFER];
    memset(writeBuffer, '\0', BUFFER);
    for(int i = 0; i<length;i+=BUFFER)
    {
        if(length-i>=BUFFER)
        {
            memcpy(writeBuffer, &word[i], BUFFER);
            if(DEBUG)printf("writing:|%s| up to %d\n",writeBuffer, BUFFER);
            write(wd, writeBuffer, BUFFER);
        }
        else
        {
            memcpy(writeBuffer, &word[i], length-i);
            if(DEBUG)printf("writing:|%s| up to %d\n",writeBuffer, length-i);
            write(wd, writeBuffer, length-i);
        }
    }
    close(wd);
    return 1;
}

int main(int argc, char** argv) 
{
    if(argc == 2)
    {
        //TODO: read from standard in
    }
    else if(argc ==3)
    {
        struct stat arguementDirectory;
        if (stat(argv[2], &arguementDirectory))//checks if a file a valid
        {
            perror("ERROR");
            return EXIT_FAILURE;
        }
        if(S_ISREG(arguementDirectory.st_mode))//regular file
        {
            if(DEBUG) printf("%s is a normal file\n", argv[2]);
        }
        else if(S_ISDIR(arguementDirectory.st_mode))//directory
        {
            if(DEBUG) printf("%s is a Directory\n", argv[2]);
            
            DIR *givenDir = opendir(argv[2]);
            struct dirent *currDir;
            currDir = readdir(givenDir);
            stat(argv[2], &arguementDirectory);
            struct stat checkDir;
            while(currDir!=NULL)
            {
                if(DEBUG)printf("checking dir: %s \n", currDir->d_name);
                if(strlen(currDir->d_name)==0)
                {
                    //TODO: if the length of the file name is 0
                    currDir = readdir(givenDir);
                    continue;
                }
                if(currDir->d_name[0]=='.')
                {
                    if(DEBUG)printf("ignored for ww bc of .: %s \n",currDir->d_name);
                    currDir = readdir(givenDir);
                    continue;
                }
                if(strlen(currDir->d_name)>5)
                {
                    if(currDir->d_name[0]=='w'&&currDir->d_name[1]=='r'&&currDir->d_name[2]=='a'&&currDir->d_name[3]=='p'&&currDir->d_name[4]=='.')
                    {
                        if(DEBUG)printf("ignored for ww bc of wrap.: %s \n",currDir->d_name);
                        currDir = readdir(givenDir);
                        continue;
                    }
                }
                
                char* temp = (char*)malloc(sizeof(char)*strlen(argv[2])+1+sizeof(char)*strlen(currDir->d_name)+1);
                memcpy(temp,argv[2],strlen(argv[2]));
                temp[sizeof(char)*strlen(argv[2])] = '/';
                memcpy(&temp[strlen(argv[2])+1],currDir->d_name,strlen(currDir->d_name));
                temp[sizeof(char)*strlen(argv[2])+1+sizeof(char)*strlen(currDir->d_name)] = '\0';
                stat(temp, &checkDir);

                if(S_ISREG(checkDir.st_mode))
                {
                    if(DEBUG)printf("ww this file: %s \n",currDir->d_name);
                }
                else if(S_ISDIR(checkDir.st_mode))
                {
                    if(DEBUG)printf("ignored for ww bc it is a directory: %s \n",currDir->d_name);
                }
                
                free(temp);
                currDir = readdir(givenDir);
            }
        }
        else
        {
            printf("ERROR: Not a regular file or a directory");
            return EXIT_FAILURE;
        }
    }
    else
    {
        printf("ERROR: Invalid number of arguments\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

