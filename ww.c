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

typedef enum bool { false = 0, true = 1 } bool;

int wrap(int fd, int line_length, char *out) {
	int pstage;
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
				if (linelen + wordlen > line_length && out[pos - 1] != '\n') { // Word (when added) will exceed wrap length, so add newline
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
	out[pos] = '\0';
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
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
        if(S_ISREG(arguementDirectory.st_mode))//checks if it is a single regular file
        {
            if(DEBUG) printf("%s is a normal file\n", argv[2]);
        }
        else if(S_ISDIR(arguementDirectory.st_mode))//checks if it is a directory
        {
            if(DEBUG) printf("%s is a Directory\n", argv[2]);
            
            DIR *givenDir = opendir(argv[2]);
            struct dirent *currDir;
            currDir = readdir(givenDir);
            stat(argv[2], &arguementDirectory);
            struct stat checkDir;
            while(currDir!=NULL)//go through the directory
            {
                if(DEBUG)printf("checking dir: %s \n", currDir->d_name);
                if(strlen(currDir->d_name)==0)
                {
                    //TODO: if the length of the file name is 0
                    currDir = readdir(givenDir);
                    continue;
                }
                if(currDir->d_name[0]=='.')//. case
                {
                    if(DEBUG)printf("ignored for ww bc of .: %s \n",currDir->d_name);
                    currDir = readdir(givenDir);
                    continue;
                }
                if(strlen(currDir->d_name)>5)//.wrap case
                {
                    if(currDir->d_name[0]=='w'&&currDir->d_name[1]=='r'&&currDir->d_name[2]=='a'&&currDir->d_name[3]=='p'&&currDir->d_name[4]=='.')
                    {
                        if(DEBUG)printf("ignored for ww bc of wrap.: %s \n",currDir->d_name);
                        currDir = readdir(givenDir);
                        continue;
                    }
                }
                
                //temporairly make a path
                char* temp = (char*)malloc(sizeof(char)*strlen(argv[2])+1+sizeof(char)*strlen(currDir->d_name)+1);
                memcpy(temp,argv[2],strlen(argv[2]));
                temp[sizeof(char)*strlen(argv[2])] = '/';
                memcpy(&temp[strlen(argv[2])+1],currDir->d_name,strlen(currDir->d_name));
                temp[sizeof(char)*strlen(argv[2])+1+sizeof(char)*strlen(currDir->d_name)] = '\0';
                stat(temp, &checkDir);

                if(S_ISREG(checkDir.st_mode))//Check if a directory is file
                {
                    char* tempWrap = (char*)malloc(sizeof(char)*strlen(argv[2])+1+sizeof(char)*strlen(currDir->d_name)+1+5);
                    memcpy(tempWrap,argv[2],strlen(argv[2]));
                    char *p = "/wrap.";
                    memcpy(&tempWrap[strlen(argv[2])],p,6);
                    memcpy(&tempWrap[strlen(argv[2])+6],currDir->d_name,strlen(currDir->d_name));
                    tempWrap[sizeof(char)*strlen(argv[2])+6+sizeof(char)*strlen(currDir->d_name)] = '\0';

                    if(DEBUG)printf("ww this file: %s into %s\n",currDir->d_name, tempWrap);
                    //TODO add the writeWW method along with wrap.
                    
                    char* word = NULL;//wrap();
                    
                    //writeWW(word, tempWrap);

                    free(tempWrap);
                }
                else if(S_ISDIR(checkDir.st_mode))//check if a directory is a folder
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
