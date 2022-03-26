#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>


#define DEBUG 1

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

char* wrap(FILE* f) 
{

}
