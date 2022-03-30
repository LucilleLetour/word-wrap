#include <stdlib.h>
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
#define READBUFFER 120

typedef enum bool { false = 0, true = 1 } bool;

int ww(int fdr, int fdw, int line_length)
{
    if(DEBUG)printf("fdr: %d, fdw: %d, line_length: %d \n", fdr,fdw,line_length);
    write(fdw, "hello", 5);
    return EXIT_SUCCESS;
}

int wrap(int fd, int line_length, char *out) {
	int pstage = 0;
	char c[1];
	char word[1024];
	int wordlen = 0;
	int linelen = 0;
	int pos = 0;
	bool failed = false;
	while (read(fd, c, 1)) {
		if (c[0] == ' ' || c[0] == '\n') { // Handle ws
			if (wordlen > 0) { // Add the word that's been being built first
				if (wordlen > line_length) { // The word is longer than the wrap length; report failure
					failed = true;
				}
				if (pos > 0 && linelen + wordlen > line_length && out[pos - 1] != '\n') { // Word (when added) will exceed wrap length, so add newline (unless it is the first word)
					if(out[pos - 1] == ' ') {
						out[pos - 1] = '\n'; // Convert ending space to newline
					} else {
						out[pos] = '\n';
						pos++;
					}
					linelen = 0;
				}
				for (int i = 0; i < wordlen; i++) { // Add in the word
					out[pos] = word[i];
					pos++;
					linelen++;
				}
				wordlen = 0;
			}
			if (pos == 0) continue;

			if (c[0] == '\n') {
				if(pstage == 1) { // Previous character was also newline, must make new paragraph
					out[pos] = '\n';
					pos++;
					if(out[pos - 2] != '\n') {
						out[pos] = '\n';
						pos++;
					}
					linelen = 0;
					pstage = 2; // Directly proceding newlines are ignored
				} else if(pstage == 0) {
					pstage = 1;
				}
			} else if (c[0] == ' ') {
				if (pos != 0 && out[pos - 1] != ' ' && out[pos - 1] != '\n' && linelen <= line_length) { // Add space char if appropriate: no preceding whitespace and not overrunning line length
					out[pos] = ' ';
					pos++;
					linelen++;
				}
			}
		} else {
			if(pstage == 1) {
				if(c[0] != ' ' && out[pos - 1] != ' ' && out[pos - 1] != '\n') { // Add a space instead of the newline, if it's needed
			 		out[pos] = ' ';
					pos++;
					linelen++;
				}
			}
			pstage = 0;
			word[wordlen] = c[0];
			wordlen++;
		}
	}
	if (wordlen > 0) { // Repeat of above, add word into file in case EOF with word in buffer
		if (wordlen > line_length) {
			failed = true;
		}
		if (linelen + wordlen > line_length) {
			out[pos] = '\n';
			linelen = 0;
			pos++;
		}
		for (int i = 0; i < wordlen; i++) {
			out[pos] = word[i];
			pos++;
			linelen++;
		}
		wordlen = 0;
	}
	while (pos > 0 && (out[pos - 1] == '\n' || out[pos - 1] == ' ')) { // Strip trailing whitespace
		pos--;
	}
	out[pos] = '\0';
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

int dirCheck(char* givenDirectory)
{
    struct stat arguementDirectory;
    if (stat(givenDirectory, &arguementDirectory))//checks if a file a valid
    {
        perror(givenDirectory);
        return -1;
    }
    else if(S_ISREG(arguementDirectory.st_mode))//checks if it is a single regular file
    {
        if(DEBUG) printf("%s is a normal file so ww\n", givenDirectory);
        return 0;
    }
    else if(S_ISDIR(arguementDirectory.st_mode))//checks if it is a directory
    {
        if(DEBUG) printf("%s is a Directory\n", givenDirectory);
        return 1;
    }
    return -1;
}

int multDir(char* givenDirectory, int line_length)
{
    DIR *givenDir = opendir(givenDirectory);
    struct dirent *currDir;

    currDir = readdir(givenDir);
    struct stat checkDir;

    chdir(givenDirectory);

    bool failed = false;

    while(currDir!=NULL)
    {
        if(DEBUG)printf("checking dir: %s \n", currDir->d_name);
        stat(currDir->d_name, &checkDir);
        if(currDir->d_name[0]=='.')//. case
        {
            if(DEBUG)printf("ignored for ww bc of .: %s \n",currDir->d_name);
            currDir = readdir(givenDir);
            continue;
        }
        else if(strlen(currDir->d_name)>5)//.wrap case
        {
            if(currDir->d_name[0]=='w'&&currDir->d_name[1]=='r'&&currDir->d_name[2]=='a'&&currDir->d_name[3]=='p'&&currDir->d_name[4]=='.')
            {
                if(DEBUG)printf("ignored for ww bc of wrap.: %s \n",currDir->d_name);
                currDir = readdir(givenDir);
                continue;
            }
        }
        else if(S_ISDIR(checkDir.st_mode))//check if a directory is a folder
        {
            if(DEBUG)printf("ignored for ww bc it is a directory: %s \n",currDir->d_name);
            currDir = readdir(givenDir);
            continue;
        }

        if(S_ISREG(checkDir.st_mode))//Check if a directory is file
        {
            char* wrap = "wrap.";
            char* end = "\0";
            char* tempWrap = (char*) malloc(5+strlen(currDir->d_name)+1);
            memcpy(tempWrap, wrap, 5);
            memcpy(&tempWrap[5], currDir->d_name, strlen(currDir->d_name));
            memcpy(&tempWrap[5+strlen(currDir->d_name)], end, 1);

            if(DEBUG)printf("ww this file: |%s| to |%s|\n", currDir->d_name,tempWrap);

            int fdr = open(currDir->d_name, O_RDONLY);
            int fdw = open(tempWrap, O_RDWR | O_CREAT |O_APPEND, S_IRUSR|S_IWUSR);
            int ret = ww(fdr, fdw, line_length);
            if (ret == EXIT_FAILURE)
            {
                failed = true;
            }
            free(tempWrap);
            close(fdr);
            close(fdw);
        }
        currDir = readdir(givenDir);
    }
    closedir(givenDir);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char** argv) 
{
    if(argc == 2)
    {
        int line_length = atoi(argv[1]);
        if(line_length <1)
        {
            printf("ERROR: Invalid Length\n");
            return EXIT_FAILURE;
        }
        int ret = ww(0,1,line_length);
        if(ret == EXIT_FAILURE)
        {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    else if(argc == 3)
    {
        int line_length = atoi(argv[1]);
        if(line_length <1)
        {
            printf("ERROR: Invalid Length\n");
            return EXIT_FAILURE;
        }
        int option = dirCheck(argv[2]);
        if(option==-1)
        {
            printf("ERROR: Invalid Directory\n");
            return EXIT_FAILURE;
        }
        else if(option==0)
        {
            if(DEBUG)printf("|%s|: Normal file\n",argv[2]);
            int fdr = open(argv[2], O_RDONLY);
            if(fdr==-1)
            {
                perror(argv[2]);
                return EXIT_FAILURE;
            }
            int ret = ww(fdr,1,line_length);
            close(fdr);
            if(ret == EXIT_FAILURE)
            {
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }
        else if(option==1)
        {
            if(DEBUG)printf("|%s|: Directory",argv[2]);
            int ret = multDir(argv[2], line_length);
            if(ret == EXIT_FAILURE)
            {
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }
    }
    else
    {
        printf("ERROR: Invalid number of arguments\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}